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

#include <tuple>
#include <vector>
#include <limits>
#include <algorithm>
#include <cmath>

#include "include/extractKernels.hpp"

tuple<vector<int>, vector<int>, int, int> divideNumTauBlocks(const int numTau, const int maxThreads, const int maxComputableTau) {

	// In this version of the function we will return constant sized local tiles
	// This is an area for further optimization

	// We want to find numTau1Local and numTau2Local such that
	// 1) max(numTau1local,numTau2local) is small relative to maxComputableTau (and definitely not greater)
	// 2) Their product divided by 16 is less than or equal to maxThreads
	// 3) numTau2local is divisible by 16
	// 4) Tiling them into maxTau does not take too many blocks

	const int numTaus = maxThreads * 16;

	float minCost = numeric_limits<float>::infinity();
	int minCostTau1 = 0;
	int minCostTau2 = 0;

	for (int testTau1 = 1; testTau1 <= min({ maxThreads, maxComputableTau, numTau }); testTau1++) {
		for (int testTau2 = 16;
			((testTau1*testTau2 / 16) <= maxThreads)	& // 2)
			(testTau2 <= maxComputableTau)				& // 1)
			(testTau2 < numTau + 16); // Skip unreasonable tau2s
			testTau2 += 16) {
			float relativeCostMaxTauTooHigh = float(maxComputableTau) / float(maxComputableTau - max(testTau1, testTau2));
			float numTilesTau1 = ceil(float(numTau) / float(testTau1));
			float numTilesTau2 = ceil(float(numTau) / float(testTau2));
			float cost = relativeCostMaxTauTooHigh*numTilesTau1*numTilesTau2;
			if (cost < minCost) {
				minCost = cost;
				minCostTau1 = testTau1;
				minCostTau2 = testTau2;
			}
		}
	}

	vector<int> tau1Offsets, tau2Offsets, numTau1Locals, numTau2Locals;
	for (int tau1Offset = 0; tau1Offset < numTau; tau1Offset += minCostTau1)
		for (int tau2Offset = 0; tau2Offset < numTau; tau2Offset += minCostTau2) {
			tau1Offsets.push_back(tau1Offset);
			tau2Offsets.push_back(tau2Offset);
		}

	return tuple<vector<int>, vector<int>, int, int> {tau1Offsets, tau2Offsets, minCostTau1, minCostTau2};
}
