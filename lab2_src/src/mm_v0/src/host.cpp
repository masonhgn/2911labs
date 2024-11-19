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

#include <iostream>
#include <cstring>
#include <time.h>
#include <algorithm>
#include <vector>
#include <omp.h>

// XRT includes
#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"

typedef short DTYPE;
const int SIZE = 512;

void mm_sw( std::vector<DTYPE> A, std::vector<DTYPE> B, std::vector<DTYPE> & AB){

#pragma omp parallel
    {
        int tid = omp_get_thread_num();
        if( tid == 0 ){
            int nthreads = omp_get_num_threads();
            std::cout << "Running OpenMP with " << nthreads << " threads...\n";
        }
    }

    DTYPE sum = 0;
#pragma omp parallel for private(sum)
    for(int i = 0; i < SIZE; i++){
        for(int j = 0; j<SIZE; j++){
            sum = 0;
            for(int k = 0; k < SIZE; k++){
                sum = sum + A[i*SIZE+k] * B[k*SIZE+j];
            }
            AB[i*SIZE+j] = sum;
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File>" << std::endl;
        return EXIT_FAILURE;
    }
    
    //////////////////////////////////////////
    // Open xclbin
    //////////////////////////////////////////
    auto device = xrt::device(0); //device index=0
    char* xclbinFilename=argv[1];
    std::cout << "Open the device " << 0 << std::endl;
    std::cout << "Load the xclbin " << xclbinFilename << std::endl;
	auto uuid = device.load_xclbin(xclbinFilename);
	auto dhdl = xrtDeviceOpenFromXcl(device);
    int matrix_size = SIZE * SIZE;
    size_t matrix_size_bytes = sizeof(DTYPE) * matrix_size;
    auto krnl = xrt::kernel(device, uuid, "mm");

    // Allocate host side memory
    std::vector<DTYPE> A(matrix_size,1);
    std::vector<DTYPE> B(matrix_size,1);
    std::vector<DTYPE> AB_sw(matrix_size,1);
    // Create the test data
    for (int i = 0; i < matrix_size; ++i) {
        A[i] = rand() % 8;
        B[i] = rand() % 8;
    }

    //Allocate Buffer in Global Memory
    auto bo0 = xrt::bo(device, matrix_size_bytes, krnl.group_id(1));
    auto bo1 = xrt::bo(device, matrix_size_bytes, krnl.group_id(1));
    auto bo_out = xrt::bo(device, matrix_size_bytes, krnl.group_id(1));

    // Map the contents of the buffer object into host memory
    auto bo0_map = bo0.map<DTYPE*>();
    auto bo1_map = bo1.map<DTYPE*>();
    auto bo_out_map = bo_out.map<DTYPE*>();

    // Create the test data
    for (int i = 0; i < matrix_size; ++i) {
        bo0_map[i] = A[i];
        bo1_map[i] = B[i];
    }

    // Synchronize buffer content with device side
    bo0.sync(XCL_BO_SYNC_BO_TO_DEVICE, matrix_size_bytes, 0);
    bo1.sync(XCL_BO_SYNC_BO_TO_DEVICE, matrix_size_bytes, 0);

    std::cout << "Running FPGA MM...\n";
    double kernel_time_in_sec = 0;
    std::chrono::duration<double> kernel_time(0);
    auto kernel_start = std::chrono::high_resolution_clock::now();

    //Execution of the kernel
    auto run = krnl(bo0, bo1, bo_out, SIZE);
    run.wait();

    auto kernel_end = std::chrono::high_resolution_clock::now();
    std::cout << "Done.\n";
    kernel_time = std::chrono::duration<double>(kernel_end - kernel_start);
    kernel_time_in_sec = kernel_time.count();
    std::cout << "Execution time = " << kernel_time_in_sec << std::endl;
    double gops = double(SIZE) * SIZE * SIZE * 2 * 1e-9 / (kernel_time_in_sec);
    std::cout << "Time: " << kernel_time_in_sec << " sec, GOPS: " << gops << std::endl;

    // Get the output data from the device;
    bo_out.sync(XCL_BO_SYNC_BO_FROM_DEVICE, matrix_size_bytes, 0);
    
    // Calculate the golden results
    mm_sw(A, B, AB_sw);

    // Validate our results
    int err_cnt = 0;
    for(int i = 0; i<SIZE; i++){
        for(int j = 0; j<SIZE; j++){
            if(AB_sw[i*SIZE+j] != bo_out_map[i*SIZE+j]) {
                err_cnt++;
                if( err_cnt == 1 ){
                    printf("i:%d j:%d sw:%d hw:%d\n", i, j, AB_sw[i*SIZE+j], bo_out_map[i*SIZE+j] );
                }
            }
        }
    }

    if(err_cnt != 0){
        printf("TEST FAILED! Error count : %d\n", err_cnt);
        return EXIT_FAILURE;
    }
    else{
        printf("TEST PASSED!\n");
    }

    return 0;
}