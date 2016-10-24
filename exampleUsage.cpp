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

#include <iostream>
#include <string>
#include <iterator>
#include <random>
#include <iomanip>
#include <algorithm>
#include <tuple>

#include "include/extractKernels.hpp"

using namespace std;

const int respLength = 10000;
const int numTau = 64;
const int numROIs = 1;
const int numSpatial = 1;
const string pathToClFiles = "clFiles/";

int main(void){

    //--------Set Up Input Data---------//
    vector< float > response (respLength*numSpatial*numROIs);
    vector< float > stimulus(respLength*numSpatial*numROIs);

    random_device rd;
	int seed = rd();
	minstd_rand random_engine(seed);
    normal_distribution<float> normalDist(0,1);

	for (int i = 0; i < response.size(); i++)
		stimulus[i] = normalDist(random_engine);
	for (int i = 2; i < response.size(); i++)
		response[i] = stimulus[i - 1] * stimulus[i - 2];

	//Vector that will contain the output kernels
	//
	vector<float> output(numTau * numSpatial * numTau * numSpatial * numROIs ,0);

	//---------Perform Extraction--------//
	int errCode;
	string errorString;
    tie(errCode,errorString) = extractKernels(response.data(), stimulus.data(),numSpatial,
											 respLength,numROIs,numTau,output.data(),pathToClFiles);

	if (errCode){
		cerr << errorString << endl;
		return EXIT_FAILURE;
	}

	//--------Output Some Values---------//
	for (int i = 0; i < 7; i++) {
		for (int j = 0; j < 7; j++) {
			cout << fixed << setprecision(3) << setw(7) << output[i * numTau * numSpatial + j] << " ";
		}
		cout << endl;
	}


	return EXIT_SUCCESS;
}
