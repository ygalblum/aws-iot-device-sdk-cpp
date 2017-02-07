#ifdef USE_WEBSOCKETS
#include "WebSocketConnection.hpp"
#elif defined USE_MBEDTLS
#include "MbedTLSConnection.hpp"
#else
#include "OpenSSLConnection.hpp"
#endif

#include "network_connection.h"
#include "network_connection.hpp"

#include "util/logging/LogMacros.hpp"
#define LOG_TAG_LANG_C "[Language - C]"

using namespace awsiotsdk;

extern "C" {

struct network_connection_s {
    std::shared_ptr<NetworkConnection> p_network_connection_;
};

static ResponseCode CreateNetworkConnection(network_connection_h net_conn,
    net_conn_params_t *net_conn_params)
{
    ResponseCode rc = ResponseCode::SUCCESS;
    util::String
        endpoint = net_conn_params->endpoint,
        root_ca_path = net_conn_params->root_ca_path,
        device_cert_path = net_conn_params->device_cert_path,
        device_private_key_path = net_conn_params->device_private_key_path;
    std::chrono::milliseconds
        tls_handshake_timeout{net_conn_params->tls_handshake_timeout},
        tls_read_timeout{net_conn_params->tls_read_timeout},
        tls_write_timeout {net_conn_params->tls_write_timeout};

#ifdef USE_WEBSOCKETS
    /*FIXME*/
    net_conn->p_network_connection_ = std::shared_ptr<NetworkConnection>(
        new network::WebSocketConnection(
        endpoint,
        net_conn_params->endpoint_port,
        root_ca_location,

        ConfigCommon::aws_region_,
        ConfigCommon::aws_access_key_id_,
        ConfigCommon::aws_secret_access_key_,
        ConfigCommon::aws_session_token_,
        ConfigCommon::tls_handshake_timeout_,
        ConfigCommon::tls_read_timeout_,
        ConfigCommon::tls_write_timeout_, true));
    if(nullptr == p_network_connection_) {
        AWS_LOG_ERROR(LOG_TAG_LANG_C, "Failed to initialize Network Connection with rc : %d", static_cast<int>(rc));
        rc = ResponseCode::FAILURE;
    }
#elif defined USE_MBEDTLS
    net_conn->p_network_connection_ =
        std::make_shared<network::MbedTLSConnection>(endpoint,
            net_conn_params->endpoint_port, root_ca_path, device_cert_path,
            device_key_path, tls_handshake_timeout, tls_read_timeout,
            tls_write_timeout, net_conn_params->server_verification_flag);
    if(nullptr == net_conn->p_network_connection_) {
        AWS_LOG_ERROR(LOG_TAG_LANG_C, "Failed to initialize Network "
            "Connection with rc : %d", static_cast<int>(rc));
        rc = ResponseCode::FAILURE;
    }
#else
    std::shared_ptr<network::OpenSSLConnection> p_network_connection =
        std::make_shared<network::OpenSSLConnection>(endpoint,
            net_conn_params->endpoint_port, root_ca_path, device_cert_path,
            device_private_key_path, tls_handshake_timeout, tls_read_timeout,
            tls_write_timeout, net_conn_params->server_verification_flag);
    rc = p_network_connection->Initialize();

    if(ResponseCode::SUCCESS != rc) {
        AWS_LOG_ERROR(LOG_TAG_LANG_C, "Failed to initialize Network "
            "Connection with rc : %d", static_cast<int>(rc));
        rc = ResponseCode::FAILURE;
    } else {
        net_conn->p_network_connection_ =
            std::dynamic_pointer_cast<NetworkConnection>(p_network_connection);
    }
#endif
    return rc;
}

void network_connection_destroy(network_connection_h network_connection)
{
    if (!network_connection) {
        return;
    }

    network_connection->p_network_connection_ = nullptr;
    free(network_connection);
}

awsiotsdk_response_code_t network_connection_create(
    net_conn_params_t *net_conn_params,
    network_connection_h *network_connection)
{
    network_connection_h net_conn = NULL;
    ResponseCode rc;

    net_conn = (network_connection_h)malloc(sizeof(*net_conn));
    if (!net_conn) {
        rc = ResponseCode::FAILURE;
        goto Error;
    }

    rc = CreateNetworkConnection(net_conn, net_conn_params);
    if (ResponseCode::SUCCESS != rc) {
        AWS_LOG_ERROR(LOG_TAG_LANG_C, "Failed to initialize network "
            "connection");
        goto Error;
    }

    *network_connection = net_conn;
    return (awsiotsdk_response_code_t)ResponseCode::SUCCESS;

Error:
    network_connection_destroy(net_conn);
    return (awsiotsdk_response_code_t)rc;
}

}

std::shared_ptr<NetworkConnection> GetNetworkConnection(
    network_connection_h network_connection)
{
    return network_connection->p_network_connection_;
}

