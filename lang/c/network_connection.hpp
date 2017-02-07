#pragma once

#include "NetworkConnection.hpp"
#include "network_connection.h"

std::shared_ptr<awsiotsdk::NetworkConnection> GetNetworkConnection(
    network_connection_h network_connection);

