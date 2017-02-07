#pragma once

#include "ResponseCode.hpp"
#include "NetworkConnection.hpp"

std::shared_ptr<awsiotsdk::NetworkConnection> GetNetworkConnection(
    network_connection_h network_connection);

