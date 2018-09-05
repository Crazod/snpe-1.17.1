//==============================================================================
//
//  Copyright (c) 2015-2017 Qualcomm Technologies, Inc.
//  All Rights Reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//==============================================================================
//
// This file contains an example application that loads and executes a neural
// network using the SNPE C++ API and saves the layer output to a file.
// Inputs to and outputs from the network are conveyed in binary form as single
// precision floating point values.
//

#include <iostream>
#include <getopt.h>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <string>
#include <iterator>
#include <unordered_map>

#include "CheckRuntime.hpp"
#include "LoadContainer.hpp"
#include "SetBuilderOptions.hpp"
#include "LoadInputTensor.hpp"
#include "ExecuteNetwork.hpp"
#include "udlExample.hpp"
#include "CreateUserBuffer.hpp"

#include "DlSystem/UserBufferMap.hpp"
#include "DlSystem/UDLFunc.hpp"
#include "DlSystem/IUserBuffer.hpp"
#include "DlContainer/IDlContainer.hpp"
#include "SNPE/SNPE.hpp"
#include "DiagLog/IDiagLog.hpp"

int main(int argc, char** argv)
{
    enum {UNKNOWN, USERBUFFER, ITENSOR};

    // Command line arguments
    static std::string dlc = "";
    static std::string OutputDir = "./output/";
    const char* inputFile = "";
    std::string bufferTypeStr = "USERBUFFER";

    // Process command line arguments
    int opt = 0;
    while ((opt = getopt(argc, argv, "hi:d:o:b:")) != -1)
    {
        switch (opt)
        {
            case 'h':
                std::cout
                    << "\nDESCRIPTION:\n"
                    << "------------\n"
                    << "Example application demonstrating how to load and execute a neural network\n"
                    << "using the SNPE C++ API.\n"
                    << "\n\n"
                    << "REQUIRED ARGUMENTS:\n"
                    << "-------------------\n"
                    << "  -d  <FILE>   Path to the DL container containing the network.\n"
                    << "  -i  <FILE>   Path to a file listing the inputs for the network.\n"
                    << "  -o  <PATH>   Path to directory to store output results.\n"
                    << "\n"
                    << "OPTIONAL ARGUMENTS:\n"
                    << "-------------------\n"
                    << "  -b  <TYPE>   Type of buffers to use [USERBUFFER, ITENSOR] (" << bufferTypeStr << "is default).\n";

                std::exit(0);
            case 'i':
                inputFile = optarg;
                break;
            case 'd':
                dlc = optarg;
                break;
            case 'o':
                OutputDir = optarg;
                break;
            case 'b':
                bufferTypeStr = optarg;
                break;
            default:
                std::cout << "Invalid parameter specified. Please run snpe-sample with the -h flag to see required arguments" << std::endl;
                std::exit(0);
        }
    }

    // Check if given arguments represent valid files
    std::ifstream dlcFile(dlc);
    std::ifstream inputList(inputFile);
    if (!dlcFile || !inputList) {
        std::cout << "Input list or dlc file not valid. Please ensure that you have provided a valid input list and dlc for processing. Run snpe-sample with the -h flag for more details" << std::endl;
        std::exit(0);
    }

    // Check if given buffer type is valid
    int bufferType;
    if (bufferTypeStr == "USERBUFFER") {
        bufferType = USERBUFFER;
    } else if (bufferTypeStr == "ITENSOR") {
        bufferType = ITENSOR;
    } else {
        std::cout << "Buffer type is not valid. Please run snpe-sample with the -h flag for more details" << std::endl;
        std::exit(0);
    }

    // Open the DL container that contains the network to execute.
    // Create an instance of the SNPE network from the now opened container.
    // The factory functions provided by SNPE allow for the specification
    // of which layers of the network should be returned as output and also
    // if the network should be run on the CPU or GPU.
    // The runtime availability API allows for runtime support to be queried.
    // If a selected runtime is not available, we will issue a warning and continue,
    // expecting the invalid configuration to be caught at SNPE network creation.
    zdl::DlSystem::UDLFactoryFunc udlFunc = sample::MyUDLFactory;
    zdl::DlSystem::UDLBundle udlBundle; udlBundle.cookie = (void*)0xdeadbeaf, udlBundle.func = udlFunc; // 0xdeadbeaf to test cookie

    static zdl::DlSystem::Runtime_t runtime = checkRuntime();
    std::unique_ptr<zdl::DlContainer::IDlContainer> container = loadContainerFromFile(dlc);
    bool useUserSuppliedBuffers = (bufferType == USERBUFFER);
    std::unique_ptr<zdl::SNPE::SNPE> snpe = setBuilderOptions(container, runtime, udlBundle, useUserSuppliedBuffers);

    // Configure logging output and start logging. The snpe-diagview
    // executable can be used to read the content of this diagnostics file
    auto logger_opt = snpe->getDiagLogInterface();
    if (!logger_opt) throw std::runtime_error("SNPE failed to obtain logging interface");
    auto logger = *logger_opt;
    auto opts = logger->getOptions();

    opts.LogFileDirectory = OutputDir;
    if(!logger->setOptions(opts)) {
        std::cerr << "Failed to set options" << std::endl;
        std::exit(1);
    }
    if (!logger->start()) {
        std::cerr << "Failed to start logger" << std::endl;
        std::exit(1);
    }

    // Open the input file listing and for each input file load its contents
    // into a SNPE tensor or user buffer, execute the network
    // with the input and save each of the returned output tensors to a file.
    size_t inputListNum = 0;
    std::string fileLine;

    if (bufferType == USERBUFFER) {
        // SNPE allows its input and output buffers that are fed to the network
        // to come from user-backed buffers. First, SNPE buffers are created from
        // user-backed storage. These SNPE buffers are then supplied to the network
        // and the results are stored in user-backed output buffers. This allows for
        // reusing the same buffers for multiple inputs and outputs.
        zdl::DlSystem::UserBufferMap inputMap, outputMap;
        std::vector<std::unique_ptr<zdl::DlSystem::IUserBuffer>> snpeUserBackedInputBuffers, snpeUserBackedOutputBuffers;
        std::unordered_map<std::string, std::vector<uint8_t>> applicationInputBuffers, applicationOutputBuffers;

        createInputBufferMap(inputMap, applicationInputBuffers, snpeUserBackedInputBuffers, snpe);
        createOutputBufferMap(outputMap, applicationOutputBuffers, snpeUserBackedOutputBuffers, snpe);

        while (std::getline(inputList, fileLine)) {
            if (fileLine.empty()) continue;
            // Load input user buffer(s) with values from file(s)
            loadInputUserBuffer(applicationInputBuffers, snpe, fileLine);
            executeNetwork(snpe, inputMap, outputMap, applicationOutputBuffers, OutputDir, inputListNum);
            ++inputListNum;
        }
    } else if (bufferType == ITENSOR) {
        while (std::getline(inputList, fileLine))
        {
            if (fileLine.empty()) continue;
            // Loading input/output buffers with ITensor
            std::unique_ptr<zdl::DlSystem::ITensor> inputTensor = loadInputTensor(snpe, fileLine);
            executeNetwork(snpe, inputTensor, OutputDir, inputListNum);
            ++inputListNum;
        }
    }
    return 0;
}
