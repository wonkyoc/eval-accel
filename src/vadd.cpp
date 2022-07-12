/**
* Copyright (C) 2019-2021 Xilinx, Inc
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/
// TRIPCOUNT indentifier
const unsigned int c_min = 4096;
const unsigned int c_max = 4 * 1024 * 1024;
const unsigned int ITER_MAX = 1000;

/*
    Vector Addition Kernel Implementation
    Arguments:
        in1   (input)     --> Input Vector1
        in2   (input)     --> Input Vector2
        out_r   (output)    --> Output Vector
        size  (input)     --> Size of Vector in Integer
*/

extern "C" {
void vadd(const unsigned int* in1, // Read-Only Vector 1
          const unsigned int* in2, // Read-Only Vector 2
          unsigned int* out_r,     // Output Result
          int size,                 // Size in integer
          int iteration
          ) {

#pragma HLS INTERFACE m_axi port = in1 bundle = gmem0
#pragma HLS INTERFACE m_axi port = in2 bundle = gmem1
#pragma HLS INTERFACE m_axi port = out_r bundle = gmem0

  unsigned int internal1[c_min];
  unsigned int internal2[c_min];
// Unoptimized vector addition kernel to increase the kernel execution time
// Large execution time required to showcase parallel execution of multiple
// compute units in this example.

mem_rd1:
  for (int i = 0; i < size; i++) {
#pragma HLS unroll
    internal1[i] = in1[i];
  }

mem_rd2:
  for (int i = 0; i < size; i++) {
#pragma HLS unroll
    internal2[i] = in2[i];
  }
  

iteration:
  for (int iter = 0; iter < iteration; iter++) {
vadd1:
      for (int i = 0; i < size; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_min max = c_max
#pragma HLS unroll
          out_r[i] = internal1[i] + internal2[i];
      }
  }
}
}
