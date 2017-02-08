#include <unordered_map>

#ifdef USE_WEBSOCKETS
#include "WebSocketConnection.hpp"
#elif defined USE_MBEDTLS
#include "MbedTLSConnection.hpp"
#else
#include "OpenSSLConnection.hpp"
#endif

#include "ResponseCode.hpp"
#include "network_connection.h"
#include "network_connection.hpp"
#include "mqtt/Client.hpp"
#include "mqtt_client.h"

#include "util/logging/Logging.hpp"
#include "util/logging/LogMacros.hpp"
#include "util/logging/ConsoleLogSystem.hpp"
#define LOG_TAG_LANG_C "[Language - C]"

using namespace awsiotsdk;

extern "C" {

class MqttMsgCtx : public mqtt::SubscriptionHandlerContextData {
    private:
        util::String m_topic;
        mqtt_msg_cb_t m_mqtt_msg_cb;
        mqtt_msg_ctx_h m_mqtt_msg_ctx;

    public:
        MqttMsgCtx(mqtt_subscribtion_t *mqtt_subscribtion)
        {
            m_topic = mqtt_subscribtion->topic;
            m_mqtt_msg_cb = mqtt_subscribtion->mqtt_msg_cb;
            m_mqtt_msg_ctx = mqtt_subscribtion->mqtt_msg_ctx;
        }

        ResponseCode Callback(util::String payload)
        {
            return (ResponseCode)m_mqtt_msg_cb(m_topic.c_str(),
                payload.c_str(), m_mqtt_msg_ctx);
        }
};

struct mqtt_ctx_s {
    std::shared_ptr<MqttClient> p_iot_client_;
    std::shared_ptr<util::Map<
        std::string, std::shared_ptr<MqttMsgCtx>>> p_mqtt_msg_ctx_map;
};

void mqtt_destroy(mqtt_ctx_h mqtt_ctx)
{
    if (nullptr == mqtt_ctx) {
        return;
    }

    mqtt_ctx->p_iot_client_ = nullptr;
    mqtt_ctx->p_mqtt_msg_ctx_map = nullptr;
    free(mqtt_ctx);
}

awsiotsdk_response_code_t mqtt_create(network_connection_h network_connection,
    uint32_t mqtt_command_timeout, mqtt_ctx_h *mqtt_ctx)
{
    mqtt_ctx_h ctx;
    ResponseCode rc;
    std::chrono::milliseconds _mqtt_command_timeout{mqtt_command_timeout};

	std::shared_ptr<awsiotsdk::util::Logging::ConsoleLogSystem> p_log_system = std::make_shared<awsiotsdk::util::Logging::ConsoleLogSystem>(awsiotsdk::util::Logging::LogLevel::Debug);
    awsiotsdk::util::Logging::InitializeAWSLogging(p_log_system);

    ctx = (mqtt_ctx_h)malloc(sizeof(*ctx));
    if (!ctx) {
        rc = ResponseCode::FAILURE;
        goto Error;
    }

    ctx->p_iot_client_ = std::shared_ptr<MqttClient>(std::move(
        MqttClient::Create(GetNetworkConnection(network_connection),
        _mqtt_command_timeout)));
    if(nullptr == ctx->p_iot_client_) {
        AWS_LOG_ERROR(LOG_TAG_LANG_C, "Failed to create an MQTT client");
        rc =  ResponseCode::FAILURE;
        goto Error;
    }

    ctx->p_mqtt_msg_ctx_map = std::shared_ptr<
        util::Map<std::string, std::shared_ptr<MqttMsgCtx>>>(
        new util::Map<std::string, std::shared_ptr<MqttMsgCtx>>);
    *mqtt_ctx = ctx;

    return (awsiotsdk_response_code_t)ResponseCode::SUCCESS;

Error:
    mqtt_destroy(ctx);
    return (awsiotsdk_response_code_t)rc;
}

awsiotsdk_response_code_t mqtt_connect(mqtt_ctx_h mqtt_ctx,
    uint32_t action_reponse_timeout, bool is_clean_session,
    mqtt_version_t mqtt_version, uint32_t keep_alive_timeout, char *client_id,
    char *username, char *password, mqtt_will_msg_t *mqtt_will_msg)
{
    std::chrono::milliseconds _action_reponse_timeout{action_reponse_timeout};
    std::chrono::seconds _keep_alive_timeout{keep_alive_timeout};
    std::unique_ptr<Utf8String>
        _client_id = client_id ? Utf8String::Create(client_id) : nullptr,
        _username = username ? Utf8String::Create(username) : nullptr,
        _password = password ? Utf8String::Create(password) : nullptr;
    std::unique_ptr<mqtt::WillOptions> _mqtt_will_msg = nullptr;
    ResponseCode rc;

    if (mqtt_will_msg) {
        std::unique_ptr<Utf8String> will_topic_name =
            Utf8String::Create(mqtt_will_msg->topic);
        util::String will_msg = mqtt_will_msg->message;
        _mqtt_will_msg = mqtt::WillOptions::Create(mqtt_will_msg->is_retained,
                (mqtt::QoS)mqtt_will_msg->qos, std::move(will_topic_name), will_msg);
    }

    rc = mqtt_ctx->p_iot_client_->Connect(_action_reponse_timeout,
        is_clean_session, (mqtt::Version)mqtt_version,
        _keep_alive_timeout, std::move(_client_id), std::move(_username),
        std::move(_password), std::move(_mqtt_will_msg));
    if (ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED != rc) {
        AWS_LOG_ERROR(LOG_TAG_LANG_C, "Failed to connect to MQTT broker");
    } else {
        rc = ResponseCode::SUCCESS;
    }

    return (awsiotsdk_response_code_t)rc;
}

awsiotsdk_response_code_t mqtt_disconnect(mqtt_ctx_h mqtt_ctx,
    uint32_t action_reponse_timeout)
{
    std::chrono::milliseconds _action_reponse_timeout{action_reponse_timeout};
    ResponseCode rc;

    rc = mqtt_ctx->p_iot_client_->Disconnect(_action_reponse_timeout);
    if (ResponseCode::SUCCESS != rc) {
        AWS_LOG_ERROR(LOG_TAG_LANG_C, "Failed to disconnect from MQTT broker");
    }

    return (awsiotsdk_response_code_t)rc;
}

awsiotsdk_response_code_t mqtt_publish(mqtt_ctx_h mqtt_ctx, char *topic_name,
    bool is_retained, bool is_duplicate, mqtt_qos_t mqtt_qos, char *payload,
    uint32_t action_reponse_timeout)
{
    std::chrono::milliseconds _action_reponse_timeout{action_reponse_timeout};
    std::unique_ptr<Utf8String> _topic_name = Utf8String::Create(topic_name);
    util::String _payload = payload;
    ResponseCode rc;

    rc = mqtt_ctx->p_iot_client_->Publish(std::move(_topic_name), is_retained,
        is_duplicate, (mqtt::QoS)mqtt_qos, _payload,
        _action_reponse_timeout);
    if (ResponseCode::SUCCESS != rc) {
        AWS_LOG_ERROR(LOG_TAG_LANG_C, "Failed to publish to MQTT broker");
    }

    return (awsiotsdk_response_code_t)rc;
}

static ResponseCode msg_received(util::String topic_name,
    util::String payload,
    std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data)
{
    std::shared_ptr<MqttMsgCtx> mqttMsgCtx =
        std::dynamic_pointer_cast<MqttMsgCtx>(p_app_handler_data);
    return mqttMsgCtx->Callback(payload);
}

awsiotsdk_response_code_t mqtt_subscribe(mqtt_ctx_h mqtt_ctx,
    mqtt_subscribtion_t *mqtt_subscribtions, size_t mqtt_subscribtions_len,
    uint32_t action_reponse_timeout)
{
    std::chrono::milliseconds _action_reponse_timeout{action_reponse_timeout};
    util::Vector<std::shared_ptr<mqtt::Subscription>> topic_vector;
    ResponseCode rc;

    for (size_t i = 0; i < mqtt_subscribtions_len; ++i) {
        mqtt_subscribtion_t *mqtt_subscribtion = &mqtt_subscribtions[i];
        util::Map<std::string, std::shared_ptr<MqttMsgCtx>>::const_iterator
            itr =  mqtt_ctx->p_mqtt_msg_ctx_map->find(mqtt_subscribtion->topic);
        if(itr != mqtt_ctx->p_mqtt_msg_ctx_map->end()) {
            AWS_LOG_ERROR(LOG_TAG_LANG_C, "Already regsitered to this topic "
                "[%s]", mqtt_subscribtion->topic);
            return
                (awsiotsdk_response_code_t)ResponseCode::MQTT_SUBSCRIBE_FAILED;
        }
    }

    for (size_t i = 0; i < mqtt_subscribtions_len; ++i) {
        mqtt_subscribtion_t *mqtt_subscribtion = &mqtt_subscribtions[i];
        std::shared_ptr<MqttMsgCtx> mqttMsgCtx =
            (std::shared_ptr<MqttMsgCtx>)(new MqttMsgCtx(mqtt_subscribtion));

        std::unique_ptr<Utf8String> p_topic_name =
            Utf8String::Create(mqtt_subscribtion->topic);
        mqtt::Subscription::ApplicationCallbackHandlerPtr p_sub_handler =
            std::bind(&msg_received, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3);
        std::shared_ptr<mqtt::Subscription> p_subscription =
            mqtt::Subscription::Create(std::move(p_topic_name),
                (mqtt::QoS)mqtt_subscribtion->max_qos, p_sub_handler,
                mqttMsgCtx);
        topic_vector.push_back(p_subscription);

        mqtt_ctx->p_mqtt_msg_ctx_map->insert({mqtt_subscribtion->topic,
            mqttMsgCtx});
    }

    rc = mqtt_ctx->p_iot_client_->Subscribe(topic_vector,
        _action_reponse_timeout);
    if (ResponseCode::SUCCESS != rc) {
        AWS_LOG_ERROR(LOG_TAG_LANG_C, "Failed to subscribe to MQTT broker");

        for (size_t i = 0; i < mqtt_subscribtions_len; ++i) {
            mqtt_subscribtion_t *mqtt_subscribtion = &mqtt_subscribtions[i];
            util::String topic = mqtt_subscribtion->topic;
            mqtt_ctx->p_mqtt_msg_ctx_map->erase(topic);
        }
    }

    return (awsiotsdk_response_code_t)rc;
}

awsiotsdk_response_code_t mqtt_unsubscribe(mqtt_ctx_h mqtt_ctx, char **topics,
    size_t topics_len, uint32_t action_reponse_timeout)
{
    std::chrono::milliseconds _action_reponse_timeout{action_reponse_timeout};
    util::Vector<std::unique_ptr<Utf8String>> topic_list;
    ResponseCode rc;

    for (size_t i = 0; i < topics_len; ++i) {
        std::unique_ptr<Utf8String> p_topic_name =
            Utf8String::Create(topics[i]);
        topic_list.push_back(std::move(p_topic_name));
    }

    rc = mqtt_ctx->p_iot_client_->Unsubscribe(std::move(topic_list),
        _action_reponse_timeout);
    if (ResponseCode::SUCCESS != rc) {
        AWS_LOG_ERROR(LOG_TAG_LANG_C, "Failed to unsubscribe from MQTT broker");
    } else {
        for (size_t i = 0; i < topics_len; ++i) {
            mqtt_ctx->p_mqtt_msg_ctx_map->erase(topics[i]);
        }
    }

    return (awsiotsdk_response_code_t)rc;
}

/*TODO Add bindings for Async Publish, Subscribe and Unsubscribe */

bool mqtt_is_connected(mqtt_ctx_h mqtt_ctx)
{
    return mqtt_ctx->p_iot_client_->IsConnected();
}

void mqtt_set_auto_reconnect_enabled(mqtt_ctx_h mqtt_ctx, bool value)
{
    mqtt_ctx->p_iot_client_->SetAutoReconnectEnabled(value);
}

bool mqtt_is_auto_reconnect_enabled(mqtt_ctx_h mqtt_ctx)
{
    return mqtt_ctx->p_iot_client_->IsAutoReconnectEnabled();
}

uint32_t mqtt_get_min_reconnect_backoff_timeout(mqtt_ctx_h mqtt_ctx)
{
    return mqtt_ctx->p_iot_client_->GetMinReconnectBackoffTimeout().count();
}

void set_min_reconnect_backoff_timeout(mqtt_ctx_h mqtt_ctx,
    uint32_t min_reconnect_backoff_timeout)
{
    std::chrono::seconds
        _min_reconnect_backoff_timeout{min_reconnect_backoff_timeout};
    mqtt_ctx->p_iot_client_->SetMinReconnectBackoffTimeout(
        _min_reconnect_backoff_timeout);
}

uint32_t mqtt_get_max_reconnect_backoff_timeout(mqtt_ctx_h mqtt_ctx)
{
    return mqtt_ctx->p_iot_client_->GetMaxReconnectBackoffTimeout().count();
}

void set_max_reconnect_backoff_timeout(mqtt_ctx_h mqtt_ctx,
    uint32_t max_reconnect_backoff_timeout)
{
    std::chrono::seconds
        _max_reconnect_backoff_timeout{max_reconnect_backoff_timeout};
    mqtt_ctx->p_iot_client_->SetMaxReconnectBackoffTimeout(
        _max_reconnect_backoff_timeout);
}

} /* extern "C" */

std::shared_ptr<awsiotsdk::MqttClient> GetMqttClient(mqtt_ctx_h mqtt_ctx)
{
    return mqtt_ctx->p_iot_client_;
}

