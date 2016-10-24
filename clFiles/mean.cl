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

__kernel void mean(__global float* smallMatrices,
                  int numTimeBlocks, float numPoints) {
  size_t idx = get_global_id(0);
  size_t batchNumElements = get_global_size(0);
  float sum = 0;
	
  for (int i = 0; i < numTimeBlocks; i++)
    sum += smallMatrices[idx + i*batchNumElements];

  smallMatrices[idx] = sum/numPoints;
}