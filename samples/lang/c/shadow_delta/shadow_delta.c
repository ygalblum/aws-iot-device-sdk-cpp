#include <stdio.h>
#include <unistd.h>
#include <jansson.h>
#include <pthread.h>
#include <errno.h>

#include "network_connection.h"
#include "mqtt_client.h"
#include "shadow.h"

#define MQTT_ACTION_REQUEST_TIMEOUT (20 * 1000)
#define MQTT_KEEP_ALIVE_TIMEOUT 30

#define SHADOW_DOCUMENT_STATE_KEY "state"
#define SHADOW_DOCUMENT_REPORTED_KEY "reported"
#define SHADOW_DOCUMENT_DESIRED_KEY "desired"
#define SHADOW_DOCUMENT_VERSION_KEY "version"
#define SHADOW_DOCUMENT_TIMESTAMP_KEY "timestamp"
#define MSG_COUNT_KEY "cur_msg_count"

#define SHADOW_TOPIC_PREFIX "$aws/things/"
#define SHADOW_TOPIC_MIDDLE "/shadow/"
#define SHADOW_REQUEST_TYPE_UPDATE_STRING "update"
#define THING_NAME "CppCSDKTesting"

#define MESSAGE_COUNT 10

static awsiotsdk_response_code_t request_handler(const char *,
    shadow_request_type_t, shadow_response_type_t, const char *document);

static awsiotsdk_response_code_t g_sync_action_response;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_sync_action_cond = PTHREAD_COND_INITIALIZER;

static net_conn_params_t g_net_conn_params =  {
    "localhost",
    3004,
    "/home/blumy/repos/mqtt_local_certs/ca.crt",
    "/home/blumy/repos/mqtt_local_certs/client.crt",
    "/home/blumy/repos/mqtt_local_certs/client.key",
    60000,
    2000,
    2000,
    false
};

static shadow_subscription_t g_shadow_subscriptions[] = {
    {
        SHADOW_REQUEST_GET,
        request_handler
    },
    {
        SHADOW_REQUEST_UPDATE,
        request_handler
    },
    {
        SHADOW_REQUEST_DELETE,
        request_handler
    },
    {
        SHADOW_REQUEST_DELTA,
        request_handler
    }
};

static awsiotsdk_response_code_t request_handler(const char *thing_name,
    shadow_request_type_t request_type, shadow_response_type_t response_type,
    const char *document)
{
    switch(response_type) {
    case SHADOW_RESPONSE_ACCEPTED:
        g_sync_action_response = 300;
        break;
    case SHADOW_RESPONSE_REJECTED:
        g_sync_action_response = -908;
        break;
    case SHADOW_RESPONSE_DELTA:
        g_sync_action_response = 301;
        break;
    }

    pthread_cond_signal(&g_sync_action_cond);

    return g_sync_action_response;
}

static awsiotsdk_response_code_t perform_sync(shadow_ctx_h shadow_ctx,
    awsiotsdk_response_code_t (async_func)(shadow_ctx_h))
{
    awsiotsdk_response_code_t rc;
    struct timespec timeout = { 20, 0};
    int err;

    rc = async_func(shadow_ctx);
    if (rc) {
        goto Exit;
    }

    err = pthread_cond_timedwait(&g_sync_action_cond, &g_mutex, &timeout);
    if (err == ETIMEDOUT) {
        rc = -1;
        goto Exit;
    }

    rc = g_sync_action_response;

Exit:
    return rc;
}

int main(void)
{
    network_connection_h network_connection = NULL;
    mqtt_ctx_h mqtt_ctx = NULL;
    shadow_ctx_h shadow_ctx = NULL;
    unsigned int request_itr = 0;
    const char *doc_str;
    char *doc_dump;
    json_t *doc = NULL;
    json_t *msg_count_json = NULL;
    json_error_t json_err;
    bool locked = false;
    awsiotsdk_response_code_t rc;

    rc = network_connection_create(&g_net_conn_params, &network_connection);
    if (rc) {
        printf("Failed to create a network connection\n");
        goto Exit;
    }

    rc = mqtt_create(network_connection, MQTT_ACTION_REQUEST_TIMEOUT,
        &mqtt_ctx);
    if (rc) {
        printf("Failed to create MQTT\n");
        goto Exit;
    }

    rc = mqtt_connect(mqtt_ctx, MQTT_ACTION_REQUEST_TIMEOUT, true,
        MQTT_3_1_1, MQTT_KEEP_ALIVE_TIMEOUT, "C-Shadow-Delta", NULL, NULL,
        NULL);
    if (rc) {
        printf("Failed to connect to MQTT\n");
        goto Exit;
    }

    pthread_mutex_lock(&g_mutex);
    locked = true;

    rc = shadow_create(mqtt_ctx, MQTT_ACTION_REQUEST_TIMEOUT, THING_NAME,
        THING_NAME, &shadow_ctx);
    if (rc) {
        printf("Failed to create Shadow\n");
        goto Exit;
    }

    rc = add_shadow_subscription(shadow_ctx, g_shadow_subscriptions,
        sizeof(g_shadow_subscriptions) / sizeof(g_shadow_subscriptions[0]));
    if (rc) {
        printf("Failed to add shadow subscriptions");
        goto Exit;
    }

    do {
        rc = perform_sync(shadow_ctx, perform_get_async);
        if (300 != rc)
            break;
        rc = perform_sync(shadow_ctx, perform_delete_async);
        if (300 != rc) {
            goto Exit;
        }
    } while(0);

    doc_str = get_empty_shadow_document();
    doc = json_loads(doc_str, JSON_DECODE_ANY, &json_err);
    if (!doc) {
        printf("Failed to create JSON [%s]\n", json_err.text);
        goto Exit;
    }

    do {
        if (!request_itr) {
            doc_str = get_server_document(shadow_ctx);
            json_decref(doc);
            doc = json_loads(doc_str, JSON_DECODE_ANY, &json_err);
            if (!doc) {
                printf("Failed to create JSON [%s]\n", json_err.text);
                goto Exit;
            }
        }

        msg_count_json = json_object();
        json_object_set_new(msg_count_json, MSG_COUNT_KEY,
            json_integer(request_itr + 1));
        json_object_update(json_object_get(json_object_get(doc,
            SHADOW_DOCUMENT_STATE_KEY), SHADOW_DOCUMENT_DESIRED_KEY),
            msg_count_json);
        json_decref(msg_count_json);

        doc_dump = json_dumps(doc, 0);
        update_device_shadow(shadow_ctx, doc_dump);
        free(doc_dump);
        rc = perform_sync(shadow_ctx, perform_update_async);
        // Wait for completion
        if (!rc || rc == -908) {
            printf("Failed to update");
            goto Exit;
        }

        sleep(1);

        if (is_in_sync(shadow_ctx)) {
            printf("Shadow is in sync while expected to be out of sync\n");
        } else {
            printf("Shodow is out of sync");
        }

        doc_str = get_server_document(shadow_ctx);
        printf("Server Shodow State [%s]\n", doc_str);
        doc = json_loads(doc_str, JSON_DECODE_ANY, &json_err);

        msg_count_json = json_object();
        json_object_set_new(msg_count_json, MSG_COUNT_KEY,
            json_integer(request_itr + 1));
        json_object_update(json_object_get(json_object_get(doc,
            SHADOW_DOCUMENT_STATE_KEY), SHADOW_DOCUMENT_REPORTED_KEY),
            msg_count_json);
        json_decref(msg_count_json);

        // Update current device shadow using the above doc
        doc_dump = json_dumps(doc, 0);
        update_device_shadow(shadow_ctx, doc_dump);
        free(doc_dump);

        // Perform an Update operation
        rc = perform_sync(shadow_ctx, perform_update_async);
        // Wait for completion
        // Get Value
        if(!rc || -908 == rc) {
            goto Exit;
        }

        sleep(1);

        doc_str = get_server_document(shadow_ctx);
        printf("Server Shodow State [%s]\n", doc_str);

        if (is_in_sync(shadow_ctx)) {
            printf("Shadow is in sync\n");
        } else {
            printf("Shodow is out of sync, but should be in sync");
        }

        ++request_itr;
    } while (request_itr < MESSAGE_COUNT);

Exit:
    if (locked) {
        pthread_mutex_unlock(&g_mutex);
    }

    if (doc) {
        json_decref(doc);
    }

    if (shadow_ctx) {
        shadow_destroy(shadow_ctx);
    }

    if (mqtt_ctx) {
        mqtt_disconnect(mqtt_ctx, MQTT_ACTION_REQUEST_TIMEOUT);
        mqtt_destroy(mqtt_ctx);
    }

    if (network_connection) {
        network_connection_destroy(network_connection);
    }

    return rc;
}
