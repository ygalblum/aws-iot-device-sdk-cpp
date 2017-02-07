#include <stdio.h>
#include <unistd.h>

#include "mqtt_client.h"

#define MQTT_ACTION_REQUEST_TIMEOUT (20 * 1000)
#define MQTT_KEEP_ALIVE_TIMEOUT 30

static awsiotsdk_response_code_t msg_received(const char *topic,
    const char *payload, mqtt_msg_ctx_h msg_ctx);

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

static mqtt_subscribtion_t g_mqtt_subscribtions[] = {
    {
        .topic = "sample/pub-sub/a",
        QOS0,
        msg_received,
        NULL
    },
    {
        .topic = "sample/pub-sub/b",
        QOS0,
        msg_received,
        NULL
    }
};

static awsiotsdk_response_code_t msg_received(const char *topic,
    const char *payload, mqtt_msg_ctx_h msg_ctx)
{
    printf("Recieved MQTT message. Topic [%s] payload [%s]\n",
        topic, payload);

    return 0;
}

int main(void)
{
    mqtt_ctx_h mqtt_ctx = NULL;
    awsiotsdk_response_code_t rc;

    rc = mqtt_create(&g_net_conn_params, MQTT_ACTION_REQUEST_TIMEOUT,
        &mqtt_ctx);
    if (rc) {
        printf("Failed to create MQTT\n");
        goto Exit;
    }

    rc = mqtt_connect(mqtt_ctx, MQTT_ACTION_REQUEST_TIMEOUT, true,
        MQTT_3_1_1, MQTT_KEEP_ALIVE_TIMEOUT, "C-Pub-Sub", NULL, NULL,
        NULL);
    if (rc) {
        printf("Failed to connect to MQTT\n");
        goto Exit;
    }

    rc = mqtt_subscribe(mqtt_ctx, g_mqtt_subscribtions,
        sizeof(g_mqtt_subscribtions) / sizeof(g_mqtt_subscribtions[0]),
        MQTT_ACTION_REQUEST_TIMEOUT);
    if (rc) {
        printf("Failed to subscribe to MQTT\n");
        goto Exit;
    }

    sleep(1);

    rc = mqtt_publish(mqtt_ctx, "sample/pub-sub/b", false, false, QOS0,
        "Test", MQTT_ACTION_REQUEST_TIMEOUT);
    if (rc) {
        printf("Failed to publish to MQTT\n");
        goto Exit;
    }

    sleep(1);

Exit:
    if (mqtt_ctx) {
        mqtt_destroy(mqtt_ctx);
    }

    return rc;
}
