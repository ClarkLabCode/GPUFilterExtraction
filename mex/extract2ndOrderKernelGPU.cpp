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

#include "mex.h"
#include "matrix.h"
#include <tuple>
#include <vector>
#include <string>
#include <functional>
#include <numeric>
#include <cmath>

using namespace std;

#include "../include/extractKernels.hpp"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
	if (nrhs != 3 || !mxIsNumeric(prhs[0]) || !mxIsScalar(prhs[0])
				  || !mxIsNumeric(prhs[1])
				  || !mxIsNumeric(prhs[2]))
		mexErrMsgIdAndTxt("twodkernelextract:nrhs", "Requires scalar numTau and two matrices arrays for stimuli and responses");
	if (nlhs != 1)
		mexErrMsgIdAndTxt("twodkernelextract:nlhs", "Outputs one matrix");

	if (round(mxGetScalar(prhs[0])) != mxGetScalar(prhs[0]))
		mexErrMsgIdAndTxt("twodkernelextract:tlhs", "numTau must be an integer");

	for (int i = 1; i < 3; i++)
		if (!mxIsSingle(prhs[i]))
			mexErrMsgIdAndTxt("twodkernelextract:type", "Requires stimuli and response data of type Single");

	mwSize numTau = mwSize(mxGetScalar(prhs[0]));

	mwSize numDims = mxGetNumberOfDimensions(prhs[1]);
	mwSize respNumDims = mxGetNumberOfDimensions(prhs[2]);
	if (numDims != respNumDims || (numDims == 2 && respNumDims == 1))
		mexErrMsgIdAndTxt("twodkernelextract:numDims", "stimuli and response matrices must have the same number of dimensions");

	const mwSize* stimDims = mxGetDimensions(prhs[1]);
	const mwSize* respDims = mxGetDimensions(prhs[2]);
	for (int i = 0; i < numDims; i++) {
		if (i == 1)
			continue;
		if (stimDims[i] != respDims[i])
			mexErrMsgIdAndTxt("twodkernelextract:dims", "stimuil and response matrices must have the same dimensions except for the second");
	}

	if (respDims[1] != 1)
		mexErrMsgIdAndTxt("twodkernelextract:respDim", "response matrix's second dimension (corresponding to numStimuli) must be singleton");

	int respLength = stimDims[0];

	int numROIs;
	if (numDims < 3)
		numROIs = 1;
	else
		numROIs = accumulate(&stimDims[2], &stimDims[numDims], 1, multiplies<int>());

	int numSpatial = (numDims < 2) ? 1 : stimDims[1];

	float* stim = static_cast<float*>(mxGetData(prhs[1]));
	float* resp = static_cast<float*>(mxGetData(prhs[2]));

	vector<mwSize> outDims = {numTau*numSpatial,numTau*numSpatial};
	for (int i = 2; i < numDims; i++)
		outDims.push_back(stimDims[i]);
	mxArray* outputArray = mxCreateUninitNumericArray(outDims.size(),outDims.data(),mxSINGLE_CLASS,mxREAL);
	float* outputData = static_cast<float*>(mxGetData(outputArray));

	mxArray* path[1];
	mexCallMATLAB(1, path, 0, NULL,"getClFilePath");
	char* pathCStr;
	pathCStr = mxArrayToString(path[0]);
	string pathStr(pathCStr);
	mxFree(pathCStr);

	string errorString = "";
	int errCode;
	tie(errCode, errorString) = extractKernels(resp, stim, numSpatial, respLength, numROIs, numTau, outputData,pathStr);

	errorString.insert(0,"GPU extraction failed.\n");
	if(errCode)
		mexErrMsgIdAndTxt("twodkernelextract:extract", errorString.c_str());

	plhs[0] = outputArray;
}
