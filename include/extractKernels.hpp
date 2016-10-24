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

#include <string>
#include <vector>

#define EXIT_NO_GPU 1
#define EXIT_NOT_ENOUGH_MEMORY 2
#define EXIT_QUEUE_FAILURE 3
#define EXIT_FILE_ERROR 4
#define EXIT_OUTPUT_OVERFLOW 5

using namespace std;

tuple<int, string> extractKernels(const float respArray[], const float stimArray[], const int numStimuli,
	const int respLength, const int numROIs, const int numTau, float outputKernels[], string clPath = "");

tuple<vector<int>, vector<int>, int, int> divideNumTauBlocks(int numTau, int maxThreads, int maxComputableTau);