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

/*This is simple Example of Multiple Compute units to showcase how a single
kernel
can be instantiated into Multiple compute units. Host code will show how to use
multiple compute units and run them concurrently. */
#include "cmdlineparser.h"
#include <iostream>
#include <cstring>
#include "common.h"

// XRT includes
#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"

//#define DATA_SIZE 1024 * 64
#define num_cu 1
#define ITER_MAX 1000

const char *server_ip = "10.0.0.1";
const char *client_ip = "10.0.0.2";

//////////////MAIN FUNCTION//////////////
extern "C" {
int main(int argc, char** argv) {
    // Command Line Parser
    sda::utils::CmdLineParser parser;

    // Switches
    //**************//"<Full Arg>",  "<Short Arg>", "<Description>", "<Default>"
    parser.addSwitch("--msg_size",   "-m", "Message size", "64");
    parser.parse(argc, argv);

    // Read settings
    int data_size = stoi(parser.value("msg_size"));

    if (argc < 3) {
        parser.printHelp();
        return EXIT_FAILURE;
    }

    int key_count = 100000;
    int adjusted = key_count;
    size_t vector_size_bytes = data_size;
    auto chunk_size = vector_size_bytes / sizeof(int);

    int **bo0_map = new int*[num_cu];
    int **bo1_map = new int*[num_cu];
    int **bo_out_map = new int*[num_cu];

    // Create the test data
    //int bufReference[num_cu][chunk_size];
    for (int i = 0; i < num_cu; i++) {
      bo0_map[i] = new int[chunk_size];
      bo1_map[i] = new int[chunk_size];
      bo_out_map[i] = new int[chunk_size];
    }

    for (int i = 0; i < num_cu; i++) {
      for (int j = 0; j < chunk_size; ++j) {
          bo0_map[i][j] = j;
          bo1_map[i][j] = j;
          bo_out_map[i][j] = 0;
      }
    }

    // Socket configuration
    int sock_fd = -1;
    struct sockaddr_in server;
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    
    struct msghdr *r_msg = create_msg(data_size, &client, client_len);
    struct msghdr *s_msg = create_msg(data_size, &client, client_len);
    (void) s_msg;

    r_msg->msg_iov->iov_base = bo0_map[0];

    config_socket(&sock_fd, SOCK_DGRAM, &server, server_ip, 9999, 1);

    struct timespec t1, t2, t3, t4, t5, t6, t7, t8;

    // timestamp variable
    double total_k2u_time  = 0.0;
    double total_c2n_time  = 0.0;
    double total_memcpy_time  = 0.0;
    double total_h2d_time  = 0.0;
    double total_d2h_time  = 0.0;
    double total_proc_time = 0.0;
    double total_send_time = 0.0; 

    printf("Benchmark begin...\n");
    for (int i = 0; i < key_count; i++) {
      t1 = snap_time();
      // receive data from client
      recvmsg(sock_fd, r_msg, 0);
      t2 = snap_time();
      t3 = handle_time(r_msg, 0);

      // Synchronize buffer content with device side
      //std::cout << "synchronize input buffer data to device global memory\n";
      t4 = snap_time();
      for (int i = 1; i < num_cu; i++) {
        bo0_map[i] = bo0_map[0];
      }
      t5 = snap_time();

      // processing
      for (int iter = 0; iter < ITER_MAX; iter++) {
        for (int i = 0; i < num_cu; i++) {
          for (int j = 0; j < chunk_size; ++j) {
              bo_out_map[i][j] = bo0_map[i][j] + bo1_map[i][j];
          }
        }
      }

      t6 = snap_time();
      t7 = snap_time();

      sendto(sock_fd, bo_out_map[0], data_size, 0, (struct sockaddr *) &client, client_len);

      t8 = snap_time();

      // calculate timestamp for each operation
      total_k2u_time  += get_elapsed(t3, t2) * 1000000;
      total_c2n_time  += (get_elapsed(t1, t2) - get_elapsed(t3, t2)) * 1000000;
      total_memcpy_time  += get_elapsed(t2, t4) * 1000000;
      total_h2d_time  += get_elapsed(t4, t5) * 1000000;
      total_proc_time += get_elapsed(t5, t6) * 1000000;
      total_d2h_time  += get_elapsed(t6, t7) * 1000000;
      total_send_time += get_elapsed(t7, t8) * 1000000;
    }

    fprintf(stdout, "config=%s data_size=%d c2n_time=%.2f k2u_time=%.2f"
        " memcpy_time=%.2f h2d_time=%.2f d2h_time=%.2f proc_time=%.2f send_time=%.2f\n",
              "cpu+fpga", data_size, 
              total_c2n_time / adjusted,
              total_k2u_time / adjusted,
              total_memcpy_time / adjusted,
              total_h2d_time / adjusted,
              total_d2h_time / adjusted,
              total_proc_time / adjusted,
              total_send_time / adjusted
              );

    return 0;
}
}
