//==============================================================================
//
//  Copyright (c) 2017 Qualcomm Technologies, Inc.
//  All Rights Reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//==============================================================================

#include <iostream>
#include <algorithm>
#include <sstream>
#include <unordered_map>

#include "ExecuteNetwork.hpp"
#include "Util.hpp"

#include "SNPE/SNPE.hpp"
#include "DlSystem/ITensor.hpp"
#include "DlSystem/StringList.hpp"
#include "DlSystem/TensorMap.hpp"
#include "DlSystem/TensorShape.hpp"

void executeNetwork (std::unique_ptr<zdl::SNPE::SNPE> & snpe,
                     std::unique_ptr<zdl::DlSystem::ITensor> & input,
                     const std::string& outputDir,
                     int num)
{
    // Execute the network and store the outputs that were specified when creating the network in a TensorMap
    static zdl::DlSystem::TensorMap outputTensorMap;
    snpe->execute(input.get(), outputTensorMap);
    zdl::DlSystem::StringList tensorNames = outputTensorMap.getTensorNames();

    // Iterate through the output Tensor map, and print each output layer name
    std::for_each( tensorNames.begin(), tensorNames.end(), [&](const char* name)
    {
        std::ostringstream path;
        path << outputDir << "/"
        << "Result_" << num << "/"
        << name << ".raw";
        auto tensorPtr = outputTensorMap.getTensor(name);
        SaveITensor(path.str(), tensorPtr);
    });
}

void executeNetwork(std::unique_ptr<zdl::SNPE::SNPE>& snpe,
                    zdl::DlSystem::UserBufferMap& inputMap,
                    zdl::DlSystem::UserBufferMap& outputMap,
                    std::unordered_map<std::string,std::vector<uint8_t>>& applicationOutputBuffers,
                    const std::string& outputDir,
                    int num)
{
    // Execute the network and store the outputs in user buffers specified in outputMap
    snpe->execute(inputMap, outputMap);

    // Get all output buffer names from the network
    const zdl::DlSystem::StringList& outputBufferNames = outputMap.getUserBufferNames();

    // Iterate through output buffers and print each output to a raw file
    std::for_each(outputBufferNames.begin(), outputBufferNames.end(), [&](const char* name)
    {
       std::ostringstream path;
       path << outputDir << "/Result_" << num << "/" << name << ".raw";

       SaveUserBuffer(path.str(), applicationOutputBuffers.at(name));
    });
}
