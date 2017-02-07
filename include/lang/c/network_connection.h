#ifndef _NETWORK_CONNECTION_H_
#define _NETWORK_CONNECTION_H_

#include <stdint.h>
#include <stdbool.h>

#include "awsiotsdk_response_code.h"

#ifdef __cplusplus
extern "C" {
#endif

struct network_connection_s;
typedef struct network_connection_s *network_connection_h;

typedef struct {
    char *endpoint;
    uint32_t endpoint_port;
    char *root_ca_path;
    char *device_cert_path;
    char *device_private_key_path;
    uint32_t tls_handshake_timeout;
    uint32_t tls_read_timeout;
    uint32_t tls_write_timeout;
    bool server_verification_flag;
} net_conn_params_t;

awsiotsdk_response_code_t network_connection_create(
    net_conn_params_t *net_conn_params,
    network_connection_h *network_connection);
void network_connection_destroy(network_connection_h network_connection);

#ifdef __cplusplus
}
#endif

#endif /* _NETWORK_CONNECTION_H_ */
