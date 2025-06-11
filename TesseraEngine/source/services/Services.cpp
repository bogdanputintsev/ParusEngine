﻿#include "Services.h"

// Static initialization of services map
std::unordered_map<const std::type_info*, std::shared_ptr<tessera::Service>> tessera::Services::services;