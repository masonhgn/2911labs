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
const int PORT_WIDTH_B = 64;
const int PORT_WIDTH_b = PORT_WIDTH_B * 8;
const int DTYPE_WIDTH_b = sizeof(DTYPE) * 8;
typedef ap_int<PORT_WIDTH_b> block_t;
const int DTYPE_PER_PORT = PORT_WIDTH_B / sizeof(DTYPE);

void changeARate(hls::stream<block_t> &AStreamWide, hls::stream<DTYPE> &AStream, int N) {
	for(int ib = 0; ib < N/M; ib++) {
		for(int jb = 0; jb < N/M; jb++) {
			for(int kb = 0; kb < N/M; kb++) {
				for(int k = 0; k < M; k++) {
					for(int ii = 0; ii < M/DTYPE_PER_PORT; ii++) {
						block_t A_temp = AStreamWide.read();
						for(int i = 0; i < DTYPE_PER_PORT; i++) {
#pragma HLS pipeline II=1
							ap_int<DTYPE_WIDTH_b> val_a = A_temp(DTYPE_WIDTH_b * (i + 1) - 1, DTYPE_WIDTH_b * i);
							DTYPE a = (DTYPE) val_a;
							AStream.write(a);
						}
					}
				}
			}
		}
	}
}

void readA(block_t *A_p, hls::stream<block_t> &AStreamWide, int N) {
	for(int ib = 0; ib < N/M; ib++) {
		for(int jb = 0; jb < N/M; jb++) {
			for(int kb = 0; kb < N/M; kb++) {
				for(int k = 0; k < M; k++) {
					for(int ii = 0; ii < M/DTYPE_PER_PORT; ii++) {
#pragma HLS pipeline II=1
						AStreamWide.write(A_p[((kb*M+k)*N+ib*M)/DTYPE_PER_PORT+ii]);
					}
				}
			}
		}
	}
}

void readB(block_t *B_p, hls::stream<block_t> &BStream, int N) {
	for(int ib = 0; ib < N/M; ib++) {
		for(int jb = 0; jb < N/M; jb++) {
			for(int kb = 0; kb < N/M; kb++) {
				for(int k = 0; k < M; k++) {
					for(int jj = 0; jj < M/DTYPE_PER_PORT; jj++) {
#pragma HLS pipeline II=1
						BStream.write(B_p[((kb*M+k)*N+jb*M)/DTYPE_PER_PORT+jj]);
					}
				}
			}
		}
	}
}

void comp(hls::stream<DTYPE> &AStream, hls::stream<block_t> &BStream, hls::stream<block_t> &ABStream, int N) {
// Fill This Part !!! 

}

void writeAB(hls::stream<block_t> &ABStream, block_t *AB, int N) {
	for(int ib = 0; ib < N/M; ib++) {
		for(int jb = 0; jb < N/M; jb++) {
			for(int i = 0; i < M; i++) {
				for(int jj = 0; jj < M/DTYPE_PER_PORT; jj++) {
#pragma HLS pipeline II=1
					AB[((ib*M+i)*N+jb*M)/DTYPE_PER_PORT+jj] = ABStream.read();
				}
			}
		}
	}
}

extern "C" {
void mm(block_t *A_p,  block_t *B_p, block_t *AB_p, int N)
{


#pragma HLS INTERFACE m_axi port = A_p offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = B_p offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = AB_p offset = slave bundle = gmem2
#pragma HLS INTERFACE s_axilite port = A_p bundle = control
#pragma HLS INTERFACE s_axilite port = B_p bundle = control
#pragma HLS INTERFACE s_axilite port = AB_p bundle = control
#pragma HLS INTERFACE s_axilite port = N bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

	hls::stream<block_t> AStreamWide("AStreamWide");
	hls::stream<DTYPE> AStream("AStream");
	hls::stream<block_t> BStream("BStream");
	hls::stream<block_t> ABStream("ABStream");

#pragma HLS DATAFLOW

	readA(A_p, AStreamWide, N);
	changeARate(AStreamWide, AStream, N);
	readB(B_p, BStream, N);
	comp(AStream, BStream, ABStream, N);
	writeAB(ABStream, AB_p, N);

}

}
