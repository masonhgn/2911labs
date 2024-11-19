/**********
Copyright (c) 2019, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/

/*******************************************************************************
Description:
    HLS pragmas can be used to optimize the design : improve throughput, reduce
latency and
    device resource utilization of the resulting RTL code
    This is vector addition example to demonstrate how HLS optimizations are
used in kernel.
*******************************************************************************/

#include "hls_stream.h"
#include "ap_int.h"

typedef short DTYPE;
const int M = 256;
const TILE_SIZE = M/4;
const int PORT_WIDTH_B = 64;
const int PORT_WIDTH_b = PORT_WIDTH_B * 8;
const int DTYPE_WIDTH_b = sizeof(DTYPE) * 8;
typedef ap_int<PORT_WIDTH_b> block_t;
const int DTYPE_PER_PORT = PORT_WIDTH_B / sizeof(DTYPE);

extern "C" {

// increase port width of B_p, AB_p
void mm(DTYPE *A,  block_t *B_p, block_t *AB_p,   int N )
{
#pragma HLS INTERFACE m_axi port = A offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = B_p offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = AB_p offset = slave bundle = gmem2
#pragma HLS INTERFACE s_axilite port = A bundle = control
#pragma HLS INTERFACE s_axilite port = B_p bundle = control
#pragma HLS INTERFACE s_axilite port = AB_p bundle = control
#pragma HLS INTERFACE s_axilite port = N bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    DTYPE AB_block[M][M];
//      Add pragma to partition AB_block
#pragma HLS array_partition variable=AB_block type=block factor=TILE_SIZE;

    ib_loop: for(int ib = 0; ib < N/M; ib++) {
        jb_loop: for(int jb = 0; jb < N/M; jb++) {
            init_i_loop: for(int i = 0; i < M; i++) {
//     Add #pragma HLS pipeline II=1 pragma in init_i_loop loop, therefore, init_j_loop is fully unrolled
#pragma HLS pipeline II=1
                init_j_loop: for(int j = 0; j < M; j++) {
#pragma HLS unroll
                    AB_block[i][j] = 0;
                }
            }

            kb_loop: for(int kb = 0; kb < N/M; kb++) {
                k_loop: for(int k = 0; k < M; k++) {
                    DTYPE Bj[M];
//      Add pragma to partition Bj
#pragma HLS array_partition variable=Bj type=block factor=TILE_SIZE;
                    readB_j_loop: for(int jj = 0; jj < M/DTYPE_PER_PORT; jj++) {

#pragma HLS pipeline II=1
//       read from B_p to temp 
                        block_t B_temp = B_p[(kb*M+jj)*N+jb*M+k];

						for (int j = 0; j < DTYPE_PER_PORT; j++) {
#pragma HLS unroll
// Fill This Part !!! assign temp to Bj
                            Bj[jj * DTYPE_PER_PORT + j] = temp.range((j + 1) * DTYPE_WIDTH_b - 1, j * DTYPE_WIDTH_b);
                        
                        
							
						}
                    }

                    i_loop: for(int i = 0; i < M; i++) {
//          Add #pragma HLS pipeline II=1 pragma in i_loop loop, therefore, j is fully unrolled
#pragma HLS pipeline II=1
                        DTYPE Ai =  A[((ib*M+i)*N+kb*M)+k];
                        j_loop: for(int j = 0; j < M; j++) {
#pragma HLS unroll
                            AB_block[i][j] += Ai * Bj[j];
                        }
                    }
                }
            }

            writeAB_i_loop: for(int i = 0; i < M; i++) {
                writeAB_j_loop: for(int jj = 0; jj < M/DTYPE_PER_PORT; jj++) {
//          Add #pragma HLS pipeline II=1 pragma in writeAB_j_loop loop, therefore, j is fully unrolled
#pragma HLS pipeline II=1
					block_t AB_temp;
					for (int j = 0; j < DTYPE_PER_PORT; j++) {
#pragma HLS unroll
//                      read from AB_block to temp 
                        AB_temp.range((j + 1) * DTYPE_WIDTH_b - 1, j * DTYPE_WIDTH_b) = AB_block[i][jj * DTYPE_PER_PORT + j];
					}
//                      read from temp to AB_p 
                        AB_p[((ib * M + i) * N + jb * M + jj)] = AB_temp;

                }
            }
        }
    }
}

}