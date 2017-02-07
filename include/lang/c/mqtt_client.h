#ifndef _AWS_IOT_DEVICE_SDK_CPP_C_H_
#define _AWS_IOT_DEVICE_SDK_CPP_C_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "awsiotsdk_response_code.h"
#include "network_connection.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mqtt_ctx_s;
typedef struct mqtt_ctx_s *mqtt_ctx_h;

typedef enum {
    MQTT_3_1_1 = 4    ///< MQTT 3.1.1 (protocol message byte = 4)
} mqtt_version_t;

typedef enum {
    QOS0 = 0,	///< QoS0
    QOS1 = 1	///< QoS1
} mqtt_qos_t;

typedef struct {
    char *topic;
    char *message;
    bool is_retained;
    mqtt_qos_t qos;
} mqtt_will_msg_t;

typedef void *mqtt_msg_ctx_h;
typedef awsiotsdk_response_code_t (*mqtt_msg_cb_t)(const char *topic,
    const char *payload, mqtt_msg_ctx_h msg_ctx);

typedef struct {
    char *topic;
    mqtt_qos_t max_qos;
    mqtt_msg_cb_t mqtt_msg_cb;
    mqtt_msg_ctx_h mqtt_msg_ctx;
} mqtt_subscribtion_t;

void mqtt_destroy(mqtt_ctx_h mqtt_ctx);
awsiotsdk_response_code_t mqtt_create(network_connection_h network_connection,
    uint32_t mqtt_command_timeout, mqtt_ctx_h *mqtt_ctx);
awsiotsdk_response_code_t mqtt_connect(mqtt_ctx_h mqtt_ctx,
    uint32_t action_reponse_timeout, bool is_clean_session,
    mqtt_version_t mqtt_version, uint32_t keep_alive_timeout, char *client_id,
    char *username, char *password, mqtt_will_msg_t *mqtt_will_msg);
awsiotsdk_response_code_t mqtt_disconnect(mqtt_ctx_h mqtt_ctx,
    uint32_t action_reponse_timeout);
awsiotsdk_response_code_t mqtt_publish(mqtt_ctx_h mqtt_ctx, char *topic_name,
    bool is_retained, bool is_duplicate, mqtt_qos_t mqtt_qos, char *payload,
    uint32_t action_reponse_timeout);
awsiotsdk_response_code_t mqtt_subscribe(mqtt_ctx_h mqtt_ctx,
    mqtt_subscribtion_t *mqtt_subscribtions, size_t mqtt_subscribtions_len,
    uint32_t action_reponse_timeout);
awsiotsdk_response_code_t mqtt_unsubscribe(mqtt_ctx_h mqtt_ctx, char **topics,
    size_t topics_len, uint32_t action_reponse_timeout);

/*TODO Add bindings for Async Publish, Subscribe and Unsubscribe */

bool mqtt_is_connected(mqtt_ctx_h mqtt_ctx);
void mqtt_set_auto_reconnect_enabled(mqtt_ctx_h mqtt_ctx, bool value);
bool mqtt_is_auto_reconnect_enabled(mqtt_ctx_h mqtt_ctx);
uint32_t mqtt_get_min_reconnect_backoff_timeout(mqtt_ctx_h mqtt_ctx);
void set_min_reconnect_backoff_timeout(mqtt_ctx_h mqtt_ctx,
    uint32_t min_reconnect_backoff_timeout);
uint32_t mqtt_get_max_reconnect_backoff_timeout(mqtt_ctx_h mqtt_ctx);
void set_max_reconnect_backoff_timeout(mqtt_ctx_h mqtt_ctx,
    uint32_t max_reconnect_backoff_timeout);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _AWS_IOT_DEVICE_SDK_CPP_C_H_ */
