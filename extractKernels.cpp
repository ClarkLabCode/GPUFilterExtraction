/*
*      Copyright (C) 2016 Omer Mano
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3, or (at your option)
*  any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#define NOMINMAX
#define __CL_ENABLE_EXCEPTIONS

#ifdef __APPLE__
#include "include/cl.hpp"
#else
#include <CL/cl.hpp>
#endif

#include <utility>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <iterator>
#include <algorithm>
#include <numeric>
#include <tuple>
#include <vector>
#include <cmath>
#include "include/OpenCLErrorStrings.h"
#include "include/extractKernels.hpp"

using namespace std;

tuple<int, string> extractKernels(const float respArray[], const float stimArray[], const int numStimuli,
								  const int respLength, const int numROIs, const int numTau,
								  float outputKernels[], string clPath ) {
	string errorString;
    try {
        //------Grab the correct device---------//
        vector< cl::Platform > platformList;
        cl::Platform::get(&platformList);

		bool hasGPU = false;
		int bestPlatform = 0;
		int bestDevice = 0;
		int bestPerformance = 0;
		for (int platIdx = 0; platIdx < platformList.size(); platIdx++) {

			vector< cl::Device > devices;
			platformList[platIdx].getDevices(CL_DEVICE_TYPE_ALL,&devices);
			if (any_of(devices.begin(), devices.end(), [](cl::Device &dev) {return dev.getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_GPU; }))
				hasGPU = true;
			else
				continue; // Do not evaluate platforms that don't have a GPU
			
			// Now get devices using context method to ensure conistent indexing
			cl_context_properties cprops[3] = { CL_CONTEXT_PLATFORM,
				(cl_context_properties)(platformList[platIdx])(),
				0 };
			cl::Context context(CL_DEVICE_TYPE_GPU, cprops);
			devices = context.getInfo<CL_CONTEXT_DEVICES>();

			for (int devIdx = 0; devIdx < devices.size(); devIdx++) {
				hasGPU = true;
				string vendor = devices[devIdx].getInfo<CL_DEVICE_VENDOR>();
				int numALUsPerComputeUnit = 8;
				// This is going to be a rough estimate based on common hardware
				if (vendor.find("Intel") != std::string::npos) {
					numALUsPerComputeUnit = 8;
				}
				if (vendor.find("NVIDIA") != std::string::npos) {
					numALUsPerComputeUnit = 128;
				}
				if (vendor.find("Advanced Micro Devices") != std::string::npos) {
					numALUsPerComputeUnit = 64;
				}
				int numMADsPerSecond = devices[devIdx].getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>(); // All modern ALUs have throughput of 1
				int numComputeUnits = devices[devIdx].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();

				int relativePerformance = numComputeUnits*numALUsPerComputeUnit*numMADsPerSecond;
				if (relativePerformance > bestPerformance) {
					bestPerformance = relativePerformance;
					bestPlatform = platIdx;
					bestDevice = devIdx;
				}
			}
		}
		
		if (!hasGPU) {
			errorString.append("No GPU detected. Please check your drivers.");
			return { EXIT_NO_GPU, errorString };
		}
		
        cl_context_properties cprops[3] = { CL_CONTEXT_PLATFORM,
            (cl_context_properties)(platformList[bestPlatform])(),
            0 };
        cl::Context context(CL_DEVICE_TYPE_GPU, cprops);

        vector< cl::Device > devices;
        devices = context.getInfo<CL_CONTEXT_DEVICES>();
        cout << "Running on: " << devices[bestDevice].getInfo<CL_DEVICE_NAME>() << endl;

        //--------Compile or load kernel extraction program from opencl---------//
		
		// Dynamically determine how much local memory to use based on what's available
		const int localMemorySize = (int) devices[bestDevice].getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() - 4;
		const int numTimepoints = localMemorySize / (3 * sizeof(float));
		string defineString = "#define NUM_TIMEPOINTS " + to_string(numTimepoints) + "\n";

		vector<string> kernelNames = { "extract","mean" };
		vector<vector<char>> binaryVecs(2);
		for (int kernelNum = 0; kernelNum < 2; kernelNum++) {
			string kernelName = kernelNames[kernelNum];
			string binaryFilename = clPath + kernelName + ".bin";
			string sourceFilename = clPath + kernelName + ".cl";

			bool binaryExists = false;
			if (ifstream(binaryFilename.c_str()))
				binaryExists = true;

			vector<char> binaryVec;
			if (!binaryExists) { // Binary file does not exist: compile from source
				ifstream sourceFile(sourceFilename.c_str(), ios::in);
				if (!sourceFile.is_open()) {
					errorString.append("Could not open source file ").append(sourceFilename);
					return{ EXIT_FILE_ERROR, errorString };
				}

				string sourceStr = defineString + string(std::istreambuf_iterator<char>(sourceFile), std::istreambuf_iterator<char>());
				cl::Program::Sources ExtractSource(1, std::make_pair(sourceStr.c_str(), sourceStr.length()));
				cl::Program extractProgram(context, ExtractSource);
				try {
					const char options[] = "-cl-fast-relaxed-math";
					extractProgram.build(devices, options);
				}
				catch (cl::Error err) {
					errorString.append("Error in ");
					errorString.append(err.what());
					errorString.append(": ");
					errorString.append(getErrorString(err.err()));
					char buildlog[10000];
					extractProgram.getBuildInfo(devices[0], CL_PROGRAM_BUILD_LOG, &buildlog);
					errorString.append(buildlog);
					throw err;
				}
				//Save compiled binary to file
				const vector<size_t> binarySizes = extractProgram.getInfo<CL_PROGRAM_BINARY_SIZES>();
				vector<char*> programBinaries = extractProgram.getInfo<CL_PROGRAM_BINARIES>();
				binaryVec.assign(programBinaries[0], programBinaries[0] + binarySizes[0]);
				ofstream binFile(binaryFilename.c_str(), ios::binary);
				binFile.write((char*)binaryVec.data(), binarySizes[0]);
				binFile.close();
			}
			else { //Get the binary from the file
				ifstream binFile(binaryFilename.c_str(), ios::binary);
				binaryVec = vector<char>(std::istreambuf_iterator<char>(binFile), std::istreambuf_iterator<char>());
			}
			binaryVecs[kernelNum] = binaryVec;
		}
		cl::Program::Binaries extractBinary(1, make_pair(binaryVecs[0].data(), binaryVecs[0].size()));
		cl::Program extractProgram(context, devices, extractBinary);
		extractProgram.build();
		int error;
        cl::Kernel ExtractKernel(extractProgram, "extract", &error);

		cl::Program::Binaries meanBinary(1, make_pair(binaryVecs[1].data(), binaryVecs[1].size()));
		cl::Program meanProgram(context, devices, meanBinary);
		meanProgram.build();
		cl::Kernel MeanKernel(meanProgram, "mean", &error);

		const int64_t reservedSpace = (int64_t) 1.5e7; // Save 15 megabytes for "assign" buffers
		const int64_t totalGPUMem = devices[bestDevice].getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() - reservedSpace;
		const int64_t outputMatrixMem = int64_t(numTau*numStimuli)*int64_t(numTau*numStimuli)* sizeof(float);
		const int64_t inputMem = respLength*(numStimuli+1)* sizeof(float); //stimuli + response

		const int64_t fourGigs = (1ll << 32ll);
		if (outputMatrixMem > fourGigs) {
			errorString.append("This version of the code does not support output matrices larger than 4GB.");
			return{ EXIT_OUTPUT_OVERFLOW, errorString };
		}

		if ( 2*(outputMatrixMem + inputMem ) > totalGPUMem){
			errorString.append("This GPU does not have enough memory to calculate such a large covariance matrix \n");
			errorString.append("Total available memory: ").append(to_string(totalGPUMem)).append("\n");
			errorString.append("Input size per response = ").append(to_string(inputMem)).append("\n");
			errorString.append("Output size per response = ").append(to_string(outputMatrixMem)).append("\n");
			errorString.append("In this version of the code, at least two responses are stored on the GPU at all times");
			return{ EXIT_NOT_ENOUGH_MEMORY, errorString };
		}

		//Find each pairwise combination of the stimuli
		vector<pair<int, int>> stimuliCombs;
		for (int i = 0; i < numStimuli; i++) {
			for (int j = i; j < numStimuli; j++)
				stimuliCombs.push_back({ i,j });
		}
		const int numStimuliCombs = (int) stimuliCombs.size();

		const int maxThreads = (int) ExtractKernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(devices[0]);
		vector<int> tau1Offsets, tau2Offsets;
		int numTau1Local, numTau2Local;
		tie(tau1Offsets, tau2Offsets, numTau1Local, numTau2Local) = divideNumTauBlocks(numTau, maxThreads, numTimepoints);
		const int numTauOffsets = (int) tau1Offsets.size();

		//We want to schedule enough concurrency without using too much memory.
		//We can schedule concurrency by increasing the number of ROIs and increasing numTimeblocks
		const int deviceComputeUnits = devices[bestDevice].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
		const int workgroupsOptimalOccupancy = 8 * deviceComputeUnits;
		int optimalConcurrencyRemaining = (int) ceil(double(workgroupsOptimalOccupancy) / double(numStimuliCombs*numTauOffsets));

		const int numConcurrentROIsMemLimit = (int) (totalGPUMem / (2*(inputMem + outputMatrixMem)));
		const int numConcurrentROIsOverflowLimit = (int) ((1ll << 32ll) / outputMatrixMem);
		const int numROIsPerBatch = min({ numROIs,optimalConcurrencyRemaining,numConcurrentROIsMemLimit, numConcurrentROIsOverflowLimit });
		optimalConcurrencyRemaining = (int) ceil(double(workgroupsOptimalOccupancy)/double(numROIsPerBatch*numStimuliCombs*numTauOffsets));

		const int numTimeBlocksMemLimit = (int) ((totalGPUMem - 2*numROIsPerBatch*inputMem) / (2*numROIsPerBatch*outputMatrixMem)); // increasing numTimeblocks increases number of output matrices
		// In extract.cl, the inputs will be divided into chunks that fit into local memory
		// These chunks are then grouped into blocks, each of which is assigned to a single workgroup
		// These timeblocks can be done in parallel to improve performance
		const int maxTauLocal = max(numTau1Local-1, numTau2Local-1);
		const int timepointsComputedPerChunk = numTimepoints - maxTauLocal;
		const int numChunks = (int) ceil(double(respLength) / double(timepointsComputedPerChunk));
		const int numChunksPerBlock = (int) ceil(double(numChunks)/double(min(optimalConcurrencyRemaining, numTimeBlocksMemLimit)));
		const int numTimeBlocks = (int) ceil(double(numChunks) / double(numChunksPerBlock));

		// All workgroup concurrency will be expressed in the third dimension dimension of ndrangekernel
		const int totalConcurrency = numStimuliCombs*numTauOffsets*numROIsPerBatch*numTimeBlocks;
		vector<int> ROIAssign(totalConcurrency);
		vector<int> timeBlockAssign(totalConcurrency);
		vector<int> tau1OffsetAssign(totalConcurrency);
		vector<int> tau2OffsetAssign(totalConcurrency);
		vector<int> stim1Assign(totalConcurrency);
		vector<int> stim2Assign(totalConcurrency);
		for (int i = 0; i < totalConcurrency; i++) {
			ROIAssign[i] = i % numROIsPerBatch;
			timeBlockAssign[i] = (i / numROIsPerBatch) % numTimeBlocks;
			int tauOffsetIdx = (i / (numROIsPerBatch*numTimeBlocks)) % numTauOffsets;
			tau1OffsetAssign[i] = tau1Offsets[tauOffsetIdx];
			tau2OffsetAssign[i] = tau2Offsets[tauOffsetIdx];
			int stimIdx = i / (numROIsPerBatch*numTimeBlocks*numTauOffsets);
			stim1Assign[i] = stimuliCombs[stimIdx].first;
			stim2Assign[i] = stimuliCombs[stimIdx].second;
		}
		cl::Buffer ROIAssignBuffer			(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int)*totalConcurrency,ROIAssign.data());
		cl::Buffer timeBlockAssignBuffer	(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int)*totalConcurrency,timeBlockAssign.data());
		cl::Buffer tau1OffsetAssignBuffer	(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int)*totalConcurrency,tau1OffsetAssign.data());
		cl::Buffer tau2OffsetAssignBuffer	(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int)*totalConcurrency,tau2OffsetAssign.data());
		cl::Buffer stim1AssignBuffer		(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int)*totalConcurrency,stim1Assign.data());
		cl::Buffer stim2AssignBuffer		(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int)*totalConcurrency,stim2Assign.data());
		//Set constant kernel arguments
		ExtractKernel.setArg(2, ROIAssignBuffer);
		ExtractKernel.setArg(3, timeBlockAssignBuffer);
		ExtractKernel.setArg(4, tau1OffsetAssignBuffer);
		ExtractKernel.setArg(5, tau2OffsetAssignBuffer);
		ExtractKernel.setArg(6, stim1AssignBuffer);
		ExtractKernel.setArg(7, stim2AssignBuffer);
		ExtractKernel.setArg(8, respLength);
		ExtractKernel.setArg(9, numROIs);
		ExtractKernel.setArg(10, numChunksPerBlock);
		ExtractKernel.setArg(11, numTimeBlocks);
		ExtractKernel.setArg(12, numTau);
		ExtractKernel.setArg(13, numStimuli);
		MeanKernel.setArg(1, numTimeBlocks);
		float numTimepointsAveraged = respLength - (numTau - 1);
		MeanKernel.setArg(2, numTimepointsAveraged);

		const int numROIBatches = (int) ceil(float(numROIs) / float(numROIsPerBatch));
		const int numROIsLastBatch = (numROIs - (numROIBatches - 1)*numROIsPerBatch);

		//Declare two copies of each device buffer so we can overlap transfer and computation
		const size_t respBufferSizePerROI = respLength;
		const size_t stimBufferSizePerROI = respLength * numStimuli;
		const size_t outputElementsPerROI = numTau * numTau * numStimuli * numStimuli;

		const size_t smallMatrixBufsSize = sizeof(float) * outputElementsPerROI * numROIsPerBatch * numTimeBlocks;

		vector<cl::Buffer> respBufs, stimBufs, smallMatrixBufs;

        for (int i = 0; i < 2; i++) {
			respBufs.emplace_back(context, CL_MEM_READ_ONLY, sizeof(float)*respBufferSizePerROI * numROIsPerBatch);
            stimBufs.emplace_back(context, CL_MEM_READ_ONLY, sizeof(float)*stimBufferSizePerROI * numROIsPerBatch);
            smallMatrixBufs.emplace_back(context, CL_MEM_READ_WRITE, smallMatrixBufsSize);
        }
		

		//-------Schedule computation on GPU----------//

		cl::CommandQueue writeQueue(context, devices[0]), computeQueue(context, devices[0]), readQueue(context, devices[0]);

        vector<cl::Event> computeFinished(numROIBatches);
        vector<cl::Event> writeFinished(numROIBatches);
		vector<cl::Event> readFinished(numROIBatches);

		vector<vector<cl::Event>> writeWaitlists(numROIBatches);
		vector<vector<cl::Event>> computeWaitLists(numROIBatches);
		vector<vector<cl::Event>> readWaitLists(numROIBatches);

		for (int i = 0; i < numROIBatches; i++) {

			// Enqueue writing of inputs
			vector<cl::Event>* writeWaitlistPtr;
			if ( i < 2) {
				writeWaitlistPtr = NULL;
			}
			else {
				writeWaitlists[i] = { computeFinished[i - 2] };
				writeWaitlistPtr = &writeWaitlists[i];
			}
			const int numROIsThisBatch = (i == numROIBatches - 1) ? numROIsLastBatch : numROIsPerBatch;
			writeQueue.enqueueWriteBuffer(respBufs[i % 2], CL_FALSE, 0, sizeof(float)*respBufferSizePerROI * numROIsThisBatch, &respArray[i * respBufferSizePerROI * numROIsPerBatch], writeWaitlistPtr, NULL);
			writeQueue.enqueueWriteBuffer(stimBufs[i % 2], CL_FALSE, 0, sizeof(float)*stimBufferSizePerROI * numROIsThisBatch, &stimArray[i * stimBufferSizePerROI * numROIsPerBatch], NULL, &writeFinished[i]);
			writeQueue.flush();

			// Enqueue computation
			ExtractKernel.setArg(0, respBufs[i % 2]);
			ExtractKernel.setArg(1, stimBufs[i % 2]);
			ExtractKernel.setArg(14, smallMatrixBufs[i % 2]);

			// In this version of the code, numTau1Local and numTau2local are constant
			cl::NDRange  extractLocal(numTau1Local, numTau2Local/16, 1);
			cl::NDRange extractGlobal(numTau1Local, numTau2Local/16, totalConcurrency);

			if ( i < 2)
				computeWaitLists[i] = { writeFinished[i] };
			else
				computeWaitLists[i] = { writeFinished[i], readFinished[i - 2] };
			computeQueue.enqueueFillBuffer<float>(smallMatrixBufs[i % 2],0,0, smallMatrixBufsSize,&computeWaitLists[i],NULL);//Reset the sums to 0
			computeQueue.enqueueNDRangeKernel(ExtractKernel, cl::NullRange, extractGlobal, extractLocal, NULL, NULL);

			MeanKernel.setArg(0, smallMatrixBufs[i % 2]);
			cl::NDRange sumGlobal(outputElementsPerROI*numROIsThisBatch);
			computeQueue.enqueueNDRangeKernel(MeanKernel, cl::NullRange, sumGlobal, cl::NullRange, NULL, &computeFinished[i]);
			computeQueue.flush();

			readWaitLists[i] = { computeFinished[i] };
			readQueue.enqueueReadBuffer(smallMatrixBufs[i % 2], CL_FALSE, 0, sizeof(float)*outputElementsPerROI*numROIsThisBatch, &outputKernels[i * outputElementsPerROI * numROIsPerBatch], &readWaitLists[i], &readFinished[i]);
			readQueue.flush();
		}


        if (readQueue.finish() != CL_SUCCESS)
        {
            errorString.append("Was not able to properly finish computations");
			return{ EXIT_QUEUE_FAILURE, errorString };
        }
        else
			return{ EXIT_SUCCESS, "" };

    }
    catch (cl::Error err) {
        errorString.append("Error in ");
        errorString.append(err.what());
        errorString.append(":");
        errorString.append(getErrorString(err.err()));
		return{ EXIT_FAILURE, errorString };
    }
}
