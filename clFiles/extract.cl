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

/*
Overall, we computing a big covariance matrix with many spatial dimensions
each with many temporal dimensions. We compute a small piece of the matrix
in each OpenCL workgroup. That piece consists of a subset of the temporal
dimensions, a single pair of spatial stimuli, for a particular time block
of the response and stimuli. The matricies for different time blocks will
be summed together in a later step.
*/

__kernel void extract(__global const float* const resp_g,
					  __global const float* const stim_g,
					  __global const int* const ROIs,
					  __global const int* const timeBlockIDs,
					  __global const int* const tau1Offsets, 
					  __global const int* const tau2Offsets, 
					  __global const int* const stim1Idxs,
					  __global const int* const stim2Idxs,
					  const int respLength,
					  const int numROIs,
					  const int numChunksPerBlock,
					  const int numTimeBlocks,
					  const int numTau,
					  const int numStims,
					  __global float* const output) {

	const uint tau1 = get_global_id(0);
	const uint tau2Block = get_global_id(1);

	const size_t idx = get_global_id(2);
	const int ROI = ROIs[idx];
	const int timeBlockID = timeBlockIDs[idx];
	const int tau1Offset = tau1Offsets[idx];
	const int tau2Offset = tau2Offsets[idx];
	const int stim1Idx = stim1Idxs[idx];
	const int stim2Idx = stim2Idxs[idx];

	const int numTau1Workgroup = get_global_size(0);
	const int numTau2Workgroup = get_global_size(1) * 16;
	const int maxTauWorkgroup = max(numTau1Workgroup - 1, numTau2Workgroup - 1);
	const int maxTauGlobal = numTau-1;
	const int repeatedTimepoints = min(maxTauWorkgroup, maxTauGlobal);
	const int timepointsComputedPerChunk = NUM_TIMEPOINTS - repeatedTimepoints;
	const int startChunkThisBlock = timeBlockID*numChunksPerBlock;
	const int endChunkThisBlock = (timeBlockID + 1)*(numChunksPerBlock);

	const int localID = get_local_id(0) + get_local_id(1)*get_local_size(0);
	const int localSize = get_local_size(0)*get_local_size(1);

	union {
		float array[16];
		float16 v;
	} sum;

	//By declaring a block of local memory, we guarantee legal access
	//to small negative indexes in stim1 and stim2
	//which allows for optimizations in the main loop
	__local float const localMem[3 * NUM_TIMEPOINTS];
	__local float* const resp  = &localMem[0];
	__local float* const stim1 = &localMem[NUM_TIMEPOINTS];
	__local float* const stim2 = &localMem[2*NUM_TIMEPOINTS];

	for (uint chunkID = startChunkThisBlock; chunkID < endChunkThisBlock; chunkID++)
	{
		#pragma unroll
		for (int i = 0; i < 16; i++)
			sum.array[i] = 0;

		int startTime = chunkID*(timepointsComputedPerChunk);
 
		barrier(CLK_LOCAL_MEM_FENCE);
		
		for (int i = localID; i < NUM_TIMEPOINTS; i += localSize) {
			int t = i + startTime; //global timepoint

			//we set the response to 0 when the result of multiplication would be invalid
			if (t < maxTauGlobal || t >= respLength || i < repeatedTimepoints)
				resp[i] = 0;
			else
				resp[i] = resp_g[t + ROI*respLength];
		}

		for (int i = localID; i < NUM_TIMEPOINTS; i += localSize) {
			int t = i + startTime - tau1Offset;
			if (t < 0 || t >= respLength)
				stim1[i] = 0;
			else
				stim1[i] = stim_g[t + stim1Idx*respLength + ROI*numStims*respLength];
		}
		for (int i = localID; i < NUM_TIMEPOINTS; i += localSize) {
			int t = i + startTime - tau2Offset;
			if (t < 0 || t >= respLength)
				stim2[i] = 0;
			else
				stim2[i] = stim_g[t + stim2Idx*respLength + ROI*numStims*respLength];
		}

		barrier(CLK_LOCAL_MEM_FENCE);


		const int maxTau2 = 16 * tau2Block + 15;
		for (int t = 0; t < NUM_TIMEPOINTS; t = t+1) {
			float respStim1 = resp[t] * stim1[t - tau1];
			float16 stim2v = vload16(0, &stim2[t - maxTau2]);
			sum.v += respStim1*stim2v;
		}

		for (int i = 0; i < 16; i++) {
			//Calculate coordinate in matrix. We are using row major order internally.
			//This matrix will appear transposed in MATLAB and other column major languages
			
			const int tau2 = maxTau2 - i;
			const int globalTau1 = tau1 + tau1Offset;
			const int globalTau2 = tau2 + tau2Offset;
			const int col = globalTau1 + numTau*stim1Idx;
			const int row = globalTau2 + numTau*stim2Idx;
			const uint numCols = numTau*numStims;
			const uint numRows = numTau*numStims;
			const uint numElems = numCols*numRows;

			const uint outputIndex = col + row*numRows + ROI*numElems + timeBlockID*numROIs*numElems;
			const uint outputIndexTranspose = row + col*numRows + ROI*numElems + timeBlockID*numROIs*numElems;
			if (globalTau2 < numTau) { // Local tile may compute more than necessary
				sum.array[i] += output[outputIndex];
				output[outputIndex] = sum.array[i];
				if (stim1Idx != stim2Idx)
					output[outputIndexTranspose] = sum.array[i];
			}
		}

	}
}