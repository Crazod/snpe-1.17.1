//==============================================================================
//
//  Copyright (c) 2017 Qualcomm Technologies, Inc.
//  All Rights Reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//==============================================================================

#ifndef EXECUTE_H
#define EXECUTE_H

#include <string>
#include <unordered_map>
#include <vector>

#include "SNPE/SNPE.hpp"
#include "DlSystem/ITensor.hpp"
#include "DlSystem/UserBufferMap.hpp"

void executeNetwork(std::unique_ptr<zdl::SNPE::SNPE>& snpe,
                    std::unique_ptr<zdl::DlSystem::ITensor>& input,
                    const std::string& outputDir,
                    int num);

void executeNetwork(std::unique_ptr<zdl::SNPE::SNPE>& snpe,
                    zdl::DlSystem::UserBufferMap& inputMap,
                    zdl::DlSystem::UserBufferMap& outputMap,
                    std::unordered_map<std::string,std::vector<uint8_t>>& applicationOutputBuffers,
                    const std::string& outputDir,
                    int num);

#endif
