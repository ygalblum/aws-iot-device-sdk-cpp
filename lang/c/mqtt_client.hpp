#pragma once

#include "mqtt/Client.hpp"
#include "mqtt_client.h"

std::shared_ptr<awsiotsdk::MqttClient> GetMqttClient(mqtt_ctx_h mqtt_ctx);

