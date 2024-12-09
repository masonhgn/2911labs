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

extern "C" {

void mm(DTYPE *A,  DTYPE *B, DTYPE *AB,   int N )
{

#pragma HLS INTERFACE m_axi port = A offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = B offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = AB offset = slave bundle = gmem
#pragma HLS INTERFACE s_axilite port = A bundle = control
#pragma HLS INTERFACE s_axilite port = B bundle = control
#pragma HLS INTERFACE s_axilite port = AB bundle = control
#pragma HLS INTERFACE s_axilite port = N bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    DTYPE AB_block[M][M];

    ib_loop: for(int ib = 0; ib < N/M; ib++) {
        jb_loop: for(int jb = 0; jb < N/M; jb++) {
            init_i_loop: for(int i = 0; i < M; i++) {
                init_j_loop: for(int j = 0; j < M; j++) {
                    AB_block[i][j] = 0;
                }
            }

            kb_loop: for(int kb = 0; kb < N/M; kb++) {
                k_loop: for(int k = 0; k < M; k++) {
                    DTYPE Bj[M];
                    readB_j_loop: for(int j = 0; j < M; j++) {
                        DTYPE B_temp = B[(kb*M+k)*N+jb*M+j];
                        Bj[j] = B_temp;
                    }

                    i_loop: for(int i = 0; i < M; i++) {
                        DTYPE Ai =  A[((ib*M+i)*N+kb*M)+k];
                        j_loop: for(int j = 0; j < M; j++) {
                            AB_block[i][j] += Ai * Bj[j];
                        }
                    }
                }
            }

            writeAB_i_loop: for(int i = 0; i < M; i++) {
                writeAB_j_loop: for(int j = 0; j < M; j++) {
                    AB[(ib*M+i)*N+jb*M+j] = AB_block[i][j];
                }
            }
        }
    }
}

}
