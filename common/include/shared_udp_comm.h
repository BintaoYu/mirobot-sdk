#pragma once

#include "udp_comm.h"
#include <memory>
#include <string>

std::shared_ptr<UdpComm> acquireSharedUdpComm(const std::string& target_ip, int target_port);
