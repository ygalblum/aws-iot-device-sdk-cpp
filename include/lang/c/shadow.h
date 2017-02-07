/**
 * @file shadow.h
 * @brief This file defines the C API for a shadow type for AWS IoT Shadow operations
 *
 */
#ifndef _AWS_IOT_DEVICE_SDK_CPP_C_SHADOW_H_
#define _AWS_IOT_DEVICE_SDK_CPP_C_SHADOW_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "awsiotsdk_response_code.h"
#include "mqtt_client.h"

#ifdef __cplusplus
extern "C" {
#endif

struct shadow_ctx_s;
typedef struct shadow_ctx_s *shadow_ctx_h;

/**
 * @brief Define a type for Shadow Requests
 *
 * Documents type is not currently supported
 */
typedef enum {
    SHADOW_REQUEST_GET = 0x01,
    SHADOW_REQUEST_UPDATE = 0x02,
    SHADOW_REQUEST_DELETE = 0x04,
    SHADOW_REQUEST_DELTA = 0x08
} shadow_request_type_t;

/**
 * @brief Define a type for Shadow Responses
 */
typedef enum {
    SHADOW_RESPONSE_REJECTED = 0,
    SHADOW_RESPONSE_ACCEPTED = 1,
    SHADOW_RESPONSE_DELTA = 2
} shadow_response_type_t;

/**
 * @brief Request Handler type for Shadow requests. Called after Shadow instance proccesses incoming message
 *
 * Takes the following as parameters
 * const char * - Thing Name for which response was received
 * shadow_request_type_t - Request Type on which response was received
 * shadow_response_type_t - Response Type
 * const char * - JsonPayload of the response
 */
typedef awsiotsdk_response_code_t (*request_handler_cb)(const char *,
    shadow_request_type_t, shadow_response_type_t, const char *document);

/**
 * @brief Factory method to create Shadow instances
 *
 * @param p_mqtt_client - MQTT Client instance used for this Shadow, can NOT be changed later
 * @param mqtt_command_timeout - Timeout to use for MQTT Commands
 * @param thing_name - Thing name for this shadow
 * @param client_token_prefix - Client Token prefix to use for shadow operations
 *
 * @return std::unique_ptr to a Shadow instance
 */
awsiotsdk_response_code_t shadow_create(mqtt_ctx_h mqtt_ctx,
    uint32_t mqtt_command_timeout, const char *thing_name,
    const char *client_token_prefix, shadow_ctx_h *shadow_ctx);

void shadow_destroy(shadow_ctx_h shadow_ctx);

/**
 * @brief Update device shadow
 *
 * This function can be used to update a device shadow. The document passed here must have the same
 * structure as a device shadow document. Whatever is provided here will be merged into the device shadow json
 * with one of the below options:
 * 1) Key exists in both current state and the provided document : Provided document value is used
 * 2) Key exists only in current state : No changes, kept as is
 * 3) Key exists only in the provided document : New document's key and value is copied to device shadow json
 *
 * To easily generate a source document, please use either the GetEmptyShadowDocument function to get an empty
 * document or get either the current device state or server state document using the corrosponding functions
 *
 * @note Shadow document structure can be seen here - http://docs.aws.amazon.com/iot/latest/developerguide/thing-shadow-document.html
 *
 * @param document - JsonDocument containing the new updates
 *
 * @return ResponseCode indicating status of request
 */
awsiotsdk_response_code_t update_device_shadow(shadow_ctx_h shadow_ctx,
    const char *document);

/**
 * @brief Get reported state of the shadow on the device
 * @return JsonDocument containing a copy of the reported state of shadow that exists on the device
 */
const char *get_device_reported(shadow_ctx_h shadow_ctx);

/**
 * @brief Get desired state of the shadow on the device
 * @return JsonDocument containing a copy of the desired state of shadow that exists on the device
 */
const char *get_device_desired(shadow_ctx_h shadow_ctx);

/**
 * @brief Get state document of the shadow on the device
 * @return JsonDocument containing a copy of the shadow document that exists on the device
 */
const char *get_device_document(shadow_ctx_h shadow_ctx);

/**
 * @brief Get reported state of the shadow state received from the server
 * @return JsonDocument containing a copy of the reported state that was last received from the server
 */
const char *get_server_reported(shadow_ctx_h shadow_ctx);

/**
 * @brief Get desired state of the shadow state received from the server
 * @return JsonDocument containing a copy of the desired state that was last received from the server
 */
const char *get_server_desired(shadow_ctx_h shadow_ctx);

/**
 * @brief Get state document of the shadow state received from the server
 * @return JsonDocument containing a copy of the shadow document that was last received from the server
 */
const char *get_server_document(shadow_ctx_h shadow_ctx);

/**
 * @brief Perform a Get operation for this shadow
 *
 * If Accepted, this clears the current server shadow state and updates it with the new state received
 * from the server.  If the subscription for this request type doesn't exist, it will also subscribe to the
 * Accepted and Rejected topics.
 *
 * @return ResponseCode indicating status of the request
 */
awsiotsdk_response_code_t perform_get_async(shadow_ctx_h shadow_ctx);

/**
 * @brief Perform an Update operation for this shadow
 *
 * This function generates a diff between the current state of the shadow on the device and the last reported
 * state of the shadow on the server. Then it calls update using this diff. If the subscription for this request
 * type doesn't exist, it will also subscribe to the Accepted and Rejected topics. This does NOT automatically
 * subscribe to the delta topic.
 *
 * @return ResponseCode indicating status of the request
 */
awsiotsdk_response_code_t perform_update_async(shadow_ctx_h shadow_ctx);

/**
 * @brief Perform a Delete operation for this shadow
 *
 * If accepted, this will delete the shadow from the server. It also clears the shadow state received from the
 * server and sets it to an empty object. This will NOT affect the shadow state for the device. To do that,
 * use the UpdateDeviceShadow function with an empty document. If the subscription for this request type doesn't
 * exist, it will also subscribe to the Accepted and Rejected topics.
 *
 * @return ResponseCode indicating status of the request
 */
awsiotsdk_response_code_t perform_delete_async(shadow_ctx_h shadow_ctx);

/**
 * @brief Handle response for Get Request
 * @param response_type - Response Type received from the server
 * @param payload - Json payload received with the response
 * @return ResponseCode indicating the status of the request
 */
awsiotsdk_response_code_t handle_get_response(shadow_ctx_h shadow_ctx,
    shadow_response_type_t response_type, const char *payload);

/**
 * @brief Handle response for Update Request
 * @param response_type - Response Type received from the server
 * @param payload - Json payload received with the response
 * @return ResponseCode indicating the status of the request
 */
awsiotsdk_response_code_t handle_update_response(shadow_ctx_h shadow_ctx,
    shadow_response_type_t response_type, const char *payload);

/**
 * @brief Handle response for Delete Request
 * @param response_type - Response Type received from the server
 * @param payload - Json payload received with the response
 * @return ResponseCode indicating the status of the request
 */
awsiotsdk_response_code_t handle_delete_response(shadow_ctx_h shadow_ctx,
    shadow_response_type_t response_type, const char *payload);

/**
 * @brief Reset the timestamp generated client suffix
 */
void reset_client_token_suffix(shadow_ctx_h shadow_ctx);

/**
 * @brief Get the current version number of the shadow
 * @return uint32_t containing the current shadow version received from the server
 */
uint32_t get_current_version_number(shadow_ctx_h shadow_ctx);

/**
 * @brief Get whether the server shadow state is in sync
 *
 * @return boolean indicating sync status
 */
bool is_in_sync(shadow_ctx_h shadow_ctx);

typedef struct {
    shadow_request_type_t request_type;
    request_handler_cb cb;
} shadow_subscription_t;

/**
 * @brief Add a specific shadow subscription
 *
 * This function can be used to add a subscription to various shadow actions. Each action can optionally be
 * assigned a handler that can process any response that is received. The internal shadow state will be
 * updated before the response handler is called if it is provided.
 *
 * @param request_mapping Mapping of request type to response handler
 * @return ResponseCode indicating status of operation
 */
awsiotsdk_response_code_t add_shadow_subscription(shadow_ctx_h shadow_ctx,
    shadow_subscription_t *shadow_subscriptions,
    size_t shadow_subscriptions_len);

/**
 * @brief Static function that creates and returns an empty Shadow json document
 * @return util::JsonDocument initialized as empty shadow json document
 */
const char *get_empty_shadow_document();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _AWS_IOT_DEVICE_SDK_CPP_C_SHADOW_H_ */
