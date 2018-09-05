//==============================================================================
//
//  Copyright (c) 2017 Qualcomm Technologies, Inc.
//  All Rights Reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//==============================================================================

#ifndef LOADINPUTTENSOR_H
#define LOADINPUTTENSOR_H

#include <unordered_map>
#include <string>
#include <vector>

#include "SNPE/SNPE.hpp"
#include "DlSystem/ITensorFactory.hpp"
#include "DlSystem/TensorMap.hpp"

std::unique_ptr<zdl::DlSystem::ITensor> loadInputTensor (std::unique_ptr<zdl::SNPE::SNPE> & snpe , std::string& fileLine);
zdl::DlSystem::TensorMap loadMultipleInput (std::unique_ptr<zdl::SNPE::SNPE> & snpe , std::string& fileLine);

void loadInputUserBuffer(std::unordered_map<std::string, std::vector<uint8_t>>& applicationBuffers,
                               std::unique_ptr<zdl::SNPE::SNPE>& snpe,
                               const std::string& fileLine);

#endif
