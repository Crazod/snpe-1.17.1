//==============================================================================
//
//  Copyright (c) 2017 Qualcomm Technologies, Inc.
//  All Rights Reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//==============================================================================

#include <iostream>
#include <vector>
#include <string>
#include <assert.h>
#include <unordered_map>
#include <cstring>

#include "LoadInputTensor.hpp"
#include "Util.hpp"

#include "SNPE/SNPE.hpp"
#include "SNPE/SNPEFactory.hpp"
#include "DlSystem/ITensor.hpp"
#include "DlSystem/StringList.hpp"
#include "DlSystem/TensorMap.hpp"
#include "DlSystem/TensorShape.hpp"

// Load a single input tensor for a network which requires a single input
std::unique_ptr<zdl::DlSystem::ITensor> loadInputTensor (std::unique_ptr<zdl::SNPE::SNPE>& snpe , std::string& fileLine)
{
    std::unique_ptr<zdl::DlSystem::ITensor> input;
    const auto &strList_opt = snpe->getInputTensorNames();
    if (!strList_opt) throw std::runtime_error("Error obtaining Input tensor names");
    const auto &strList = *strList_opt;
    // Make sure the network requires only a single input
    assert (strList.size() == 1);
    // If the network has a single input, each line represents the input file to be loaded for that input

    std::string filePath(fileLine);
    std::cout << "Processing DNN Input: " << filePath << "\n";
    std::vector<float> inputVec = loadFloatDataFile(filePath);

    /* Create an input tensor that is correctly sized to hold the input of the network. Dimensions that have no fixed size will be represented with a value of 0. */

    const auto &inputDims_opt = snpe->getInputDimensions(strList.at(0));
    const auto &inputShape = *inputDims_opt;

    /* Calculate the total number of elements that can be stored in the tensor so that we can check that the input contains the expected number of elements.

       With the input dimensions computed create a tensor to convey the input into the network. */
    input = zdl::SNPE::SNPEFactory::getTensorFactory().createTensor(inputShape);

    /* Copy the loaded input file contents into the networks input tensor.SNPE's ITensor supports C++ STL functions like std::copy() */

    std::copy(inputVec.begin(), inputVec.end(), input->begin());
    return input;
}

// Load multiple input tensors for a network which require multiple inputs
zdl::DlSystem::TensorMap loadMultipleInput (std::unique_ptr<zdl::SNPE::SNPE>& snpe , std::string& fileLine)
{
    // Treat each line as a space-separated list of input files
    std::vector<std::string> filePaths;
    split(filePaths, fileLine, ' ');
    const auto& inputTensorNamesRef = snpe->getInputTensorNames();
    if (!inputTensorNamesRef) throw std::runtime_error("Error obtaining Input tensor names");
    const auto &inputTensorNames = *inputTensorNamesRef;
    // Make sure the network requires multiple inputs
    assert (inputTensorNames.size() > 1);

    if (inputTensorNames.size()) std::cout << "Processing DNN Input: " << std::endl;

    std::vector<std::unique_ptr<zdl::DlSystem::ITensor>> inputs(inputTensorNames.size());
    zdl::DlSystem::TensorMap  inputTensorMap;
    for (size_t i = 0; i<inputTensorNames.size(); i++)
    {
        // print out which file is being processed
        std::string filePath(filePaths[i]);
        std::cout << "\t" << i + 1 << ") " << filePath << std::endl;

        std::string inputName(inputTensorNames.at(i));
        std::vector<float> inputVec = loadFloatDataFile(filePath);

        const auto &inputShape_opt = snpe->getInputDimensions(inputTensorNames.at(i));
        const auto &inputShape = *inputShape_opt;
        inputs[i] = zdl::SNPE::SNPEFactory::getTensorFactory().createTensor(inputShape);
        std::copy(inputVec.begin(), inputVec.end(), inputs[i]->begin());
        inputTensorMap.add(inputName.c_str(), inputs[i].release());
    }
    std::cout << "Finished processing inputs for current inference \n";
    return inputTensorMap;
}

// Load multiple input user buffers
void loadInputUserBuffer(std::unordered_map<std::string, std::vector<uint8_t>>& applicationBuffers,
                               std::unique_ptr<zdl::SNPE::SNPE>& snpe,
                               const std::string& fileLine)
{
    // get input tensor names of the network that need to be populated
    const auto& inputNamesOpt = snpe->getInputTensorNames();
    if (!inputNamesOpt) throw std::runtime_error("Error obtaining input tensor names");
    const zdl::DlSystem::StringList& inputNames = *inputNamesOpt;
    assert(inputNames.size() > 0);

    // treat each line as a space-separated list of input files
    std::vector<std::string> filePaths;
    split(filePaths, fileLine, ' ');

    if (inputNames.size()) std::cout << "Processing DNN Input: " << std::endl;

    for (size_t i = 0; i < inputNames.size(); i++) {
        const char* name = inputNames.at(i);
        std::string filePath(filePaths[i]);

        // print out which file is being processed
        std::cout << "\t" << i + 1 << ") " << filePath << std::endl;

        // load file content onto application storage buffer,
        // on top of which, SNPE has created a user buffer
        loadByteDataFile(filePath, applicationBuffers.at(name));
    };
}


