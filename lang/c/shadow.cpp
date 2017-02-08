#include "shadow.h"
#include "mqtt_client.hpp"
#include "shadow/Shadow.hpp"

#include "util/logging/LogMacros.hpp"
#define LOG_TAG_LANG_C "[Language - C]"

using namespace awsiotsdk;

extern "C" {

class ShadowSubsription {
    private:
        request_handler_cb m_cb;
    public:
        ShadowSubsription(request_handler_cb cb)
        {
            m_cb = cb;
        }

		ResponseCode ActionResponseHandler(util::String thing_name, ShadowRequestType request_type,
            ShadowResponseType response_type, util::JsonDocument &payload)
        {
            util::String json_str = util::JsonParser::ToString(payload);

            return (ResponseCode)m_cb(thing_name.c_str(),
                shadow_request_type_t(request_type),
                shadow_response_type_t(response_type), json_str.c_str());
        }
};

struct shadow_ctx_s {
    Shadow *p_shadow;
};


void shadow_destroy(shadow_ctx_h shadow_ctx)
{
    if (nullptr == shadow_ctx) {
        return;
    }

    delete shadow_ctx->p_shadow;
    free(shadow_ctx);
}

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
    const char *client_token_prefix, shadow_ctx_h *shadow_ctx)
{
    shadow_ctx_h ctx;
    std::chrono::milliseconds _mqtt_command_timeout{mqtt_command_timeout};
    util::String _thing_name = thing_name;
    util::String _client_token_prefix = client_token_prefix;
    ResponseCode rc;

    ctx = (shadow_ctx_h)malloc(sizeof(*ctx));
    if (!ctx) {
        rc = ResponseCode::FAILURE;
        goto Error;
    }

    ctx->p_shadow = new Shadow(GetMqttClient(mqtt_ctx), _mqtt_command_timeout, _thing_name, _client_token_prefix);
    if (nullptr == ctx->p_shadow) {
        AWS_LOG_ERROR(LOG_TAG_LANG_C, "Failed to create a Shadow");
        rc =  ResponseCode::FAILURE;
        goto Error;
    }

    *shadow_ctx = ctx;

    return (awsiotsdk_response_code_t)ResponseCode::SUCCESS;

Error:
    shadow_destroy(ctx);
    return (awsiotsdk_response_code_t)rc;
}

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
    const char *document)
{
    util::JsonDocument json_document;
    util::JsonParser::InitializeFromJsonString(json_document, document);

    return (awsiotsdk_response_code_t)
        shadow_ctx->p_shadow->UpdateDeviceShadow(json_document);
}

/**
 * @brief Get reported state of the shadow on the device
 * @return JsonDocument containing a copy of the reported state of shadow that exists on the device
 */
const char *get_device_reported(shadow_ctx_h shadow_ctx)
{
    util::JsonDocument json_document =
        shadow_ctx->p_shadow->GetDeviceReported();
    util::String json_str = util::JsonParser::ToString(json_document);
    return json_str.c_str();
}

/**
 * @brief Get desired state of the shadow on the device
 * @return JsonDocument containing a copy of the desired state of shadow that exists on the device
 */
const char *get_device_desired(shadow_ctx_h shadow_ctx)
{
    util::JsonDocument json_document =
        shadow_ctx->p_shadow->GetDeviceDesired();
    util::String json_str = util::JsonParser::ToString(json_document);
    return json_str.c_str();
}

/**
 * @brief Get state document of the shadow on the device
 * @return JsonDocument containing a copy of the shadow document that exists on the device
 */
const char *get_device_document(shadow_ctx_h shadow_ctx)
{
    util::JsonDocument json_document =
        shadow_ctx->p_shadow->GetDeviceDocument();
    util::String json_str = util::JsonParser::ToString(json_document);
    return json_str.c_str();
}

/**
 * @brief Get reported state of the shadow state received from the server
 * @return JsonDocument containing a copy of the reported state that was last received from the server
 */
const char *get_server_reported(shadow_ctx_h shadow_ctx)
{
    util::JsonDocument json_document =
        shadow_ctx->p_shadow->GetServerReported();
    util::String json_str = util::JsonParser::ToString(json_document);
    return json_str.c_str();
}

/**
 * @brief Get desired state of the shadow state received from the server
 * @return JsonDocument containing a copy of the desired state that was last received from the server
 */
const char *get_server_desired(shadow_ctx_h shadow_ctx)
{
    util::JsonDocument json_document =
        shadow_ctx->p_shadow->GetServerDesired();
    util::String json_str = util::JsonParser::ToString(json_document);
    return json_str.c_str();
}

/**
 * @brief Get state document of the shadow state received from the server
 * @return JsonDocument containing a copy of the shadow document that was last received from the server
 */
const char *get_server_document(shadow_ctx_h shadow_ctx)
{
    util::JsonDocument json_document =
        shadow_ctx->p_shadow->GetServerDocument();
    util::String json_str = util::JsonParser::ToString(json_document);
    return json_str.c_str();
}

/**
 * @brief Perform a Get operation for this shadow
 *
 * If Accepted, this clears the current server shadow state and updates it with the new state received
 * from the server.  If the subscription for this request type doesn't exist, it will also subscribe to the
 * Accepted and Rejected topics.
 *
 * @return ResponseCode indicating status of the request
 */
awsiotsdk_response_code_t perform_get_async(shadow_ctx_h shadow_ctx)
{
    return (awsiotsdk_response_code_t)shadow_ctx->p_shadow->PerformGetAsync();
}

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
awsiotsdk_response_code_t perform_update_async(shadow_ctx_h shadow_ctx)
{
    return (awsiotsdk_response_code_t)
        shadow_ctx->p_shadow->PerformUpdateAsync();
}

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
awsiotsdk_response_code_t perform_delete_async(shadow_ctx_h shadow_ctx)
{
    return (awsiotsdk_response_code_t)
        shadow_ctx->p_shadow->PerformDeleteAsync();
}

/**
 * @brief Handle response for Get Request
 * @param response_type - Response Type received from the server
 * @param payload - Json payload received with the response
 * @return ResponseCode indicating the status of the request
 */
awsiotsdk_response_code_t handle_get_response(shadow_ctx_h shadow_ctx,
    shadow_response_type_t response_type, const char *payload)
{
    util::JsonDocument json_document;
    util::JsonParser::InitializeFromJsonString(json_document, payload);

    return (awsiotsdk_response_code_t)
        shadow_ctx->p_shadow->HandleGetResponse(
            ShadowResponseType(response_type), json_document);
}

/**
 * @brief Handle response for Update Request
 * @param response_type - Response Type received from the server
 * @param payload - Json payload received with the response
 * @return ResponseCode indicating the status of the request
 */
awsiotsdk_response_code_t handle_update_response(shadow_ctx_h shadow_ctx,
    shadow_response_type_t response_type, const char *payload)
{
    util::JsonDocument json_document;
    util::JsonParser::InitializeFromJsonString(json_document, payload);

    return (awsiotsdk_response_code_t)
        shadow_ctx->p_shadow->HandleUpdateResponse(
            ShadowResponseType(response_type), json_document);
}

/**
 * @brief Handle response for Delete Request
 * @param response_type - Response Type received from the server
 * @param payload - Json payload received with the response
 * @return ResponseCode indicating the status of the request
 */
awsiotsdk_response_code_t handle_delete_response(shadow_ctx_h shadow_ctx,
    shadow_response_type_t response_type, const char *payload)
{
    util::JsonDocument json_document;
    util::JsonParser::InitializeFromJsonString(json_document, payload);

    return (awsiotsdk_response_code_t)
        shadow_ctx->p_shadow->HandleDeleteResponse(
            ShadowResponseType(response_type), json_document);
}

/**
 * @brief Reset the timestamp generated client suffix
 */
void reset_client_token_suffix(shadow_ctx_h shadow_ctx)
{
    shadow_ctx->p_shadow->ResetClientTokenSuffix();
}

/**
 * @brief Get the current version number of the shadow
 * @return uint32_t containing the current shadow version received from the server
 */
uint32_t get_current_version_number(shadow_ctx_h shadow_ctx)
{
    return shadow_ctx->p_shadow->GetCurrentVersionNumber();
}

/**
 * @brief Get whether the server shadow state is in sync
 *
 * @return boolean indicating sync status
 */
bool is_in_sync(shadow_ctx_h shadow_ctx)
{
    return shadow_ctx->p_shadow->IsInSync();
}

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
    size_t shadow_subscriptions_len)
{
    util::Map<ShadowRequestType, Shadow::RequestHandlerPtr> request_mapping;

    for (size_t i = 0; i < shadow_subscriptions_len; ++i) {
        shadow_subscription_t *shadow_subscription = &shadow_subscriptions[0];

        ShadowSubsription *shadowSubsription =
            new ShadowSubsription(shadow_subscription->cb);

        Shadow::RequestHandlerPtr p_action_handler =
            std::bind(&ShadowSubsription::ActionResponseHandler,
                shadowSubsription, std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, std::placeholders::_4);

        request_mapping.insert(std::make_pair((ShadowRequestType)shadow_subscription->request_type, p_action_handler));
    }

    return (awsiotsdk_response_code_t)shadow_ctx->p_shadow->AddShadowSubscription(request_mapping);
}

/**
 * @brief Static function that creates and returns an empty Shadow json document
 * @return util::JsonDocument initialized as empty shadow json document
 */
const char *get_empty_shadow_document()
{
    util::JsonDocument json_document = Shadow::GetEmptyShadowDocument();
    util::String json_str = util::JsonParser::ToString(json_document);
    return json_str.c_str();
}

}

