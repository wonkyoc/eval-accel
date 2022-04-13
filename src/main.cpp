#include <xrt/xrt_kernel.h>
#include <xrt/xrt_bo.h>
#include "cmdlineparser.h"
#include <string>
#include "common.h"

const char *server_ip = "192.168.10.10";
const char *client_ip = "192.168.10.20";

extern "C" {
int main(int argc, char *argv[])
{
  // command line parser
  sda::utils::CmdLineParser parser;

  parser.addSwitch("--xclbin_file", "-x", "input binary file string", "");
  parser.addSwitch("--device_id",   "-d", "device index", "0");
  parser.addSwitch("--msg_size",   "-m", "Message size", "64");
  parser.parse(argc, argv);

  std::string binary_file = parser.value("xclbin_file");
  unsigned int device_index = stoi(parser.value("device_id"));
  int data_size = stoi(parser.value("msg_size"));

  if (argc < 3) {
    parser.printHelp();
    return EXIT_FAILURE;
  }

  int key_count = 100000;
  int adjusted  = key_count;
  int array_size = data_size / sizeof(int);

  auto device = xrt::device(device_index);
  auto device_name = device.get_info<xrt::info::device::name>();
 
  //std::cout << "Load the xclbin " << binary_file << std::endl;
  auto uuid = device.load_xclbin(binary_file);

  auto copy_krnl = xrt::kernel(device, uuid.get(), "copy_kernel");

  xrt::bo::flags device_flags = xrt::bo::flags::device_only;

  auto device_buffer = xrt::bo(device, data_size, device_flags, copy_krnl.group_id(0));
  auto value  = xrt::bo(device, data_size, copy_krnl.group_id(1));
  auto out    = xrt::bo(device, data_size, copy_krnl.group_id(1));

  // Map the contents of the buffer object into host memory
  auto value_map = value.map<int *>();
  auto out_map   = out.map<int *>();

  // Initialization
  std::fill(value_map, value_map + array_size, 0);
  std::fill(out_map, out_map + array_size, 0);

  // Socket configuration
  int sock_fd = -1;
  struct sockaddr_in server;
  struct sockaddr_in client;
  socklen_t client_len = sizeof(client);
  
  struct msghdr *r_msg = create_msg(data_size, &client, client_len);
  struct msghdr *s_msg = create_msg(data_size, &client, client_len);
  (void) s_msg;

  config_socket(&sock_fd, SOCK_DGRAM, &server, server_ip, 9999, 1);
  //warmup(&sock_fd, &client);

  struct timespec t1, t2, t3, t4, t5, t6, t7, t8;

  // timestamp variable
  double total_k2u_time  = 0.0;
  double total_c2n_time  = 0.0;
  double total_memcpy_time  = 0.0;
  double total_h2d_time  = 0.0;
  double total_d2h_time  = 0.0;
  double total_proc_time = 0.0;
  double total_send_time = 0.0; 

  // return to client
  int success = 1;
  char *data = (char *) malloc(data_size);

  printf("Benchmark begin...\n");
  for (int i = 0; i < key_count; i++) {
      t1 = snap_time();
      recvmsg(sock_fd, r_msg, 0);
      t2 = snap_time();
      t3 = handle_time(r_msg, 0);

      memcpy(value_map, r_msg->msg_iov->iov_base, data_size);
      t4 = snap_time();
      value.sync(XCL_BO_SYNC_BO_TO_DEVICE);

      t5 = snap_time();

      //for (int i = 0; i < data_size; i++) {
      //  data[i] = value_map[i];
      //}
      
      auto copy_run = copy_krnl(device_buffer, value, array_size, 0);
      copy_run.wait();

      t6 = snap_time();
      value.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
      t7 = snap_time();

      sendto(sock_fd, value_map, data_size, 0, (struct sockaddr *) &client, client_len);
      
      t8 = snap_time();

      total_k2u_time  += get_elapsed(t3, t2) * 1000000;
      total_c2n_time  += (get_elapsed(t1, t2) - get_elapsed(t3, t2)) * 1000000;
      total_memcpy_time  += get_elapsed(t2, t4) * 1000000;
      total_h2d_time  += get_elapsed(t4, t5) * 1000000;
      total_proc_time += get_elapsed(t5, t6) * 1000000;
      total_d2h_time  += get_elapsed(t6, t7) * 1000000;
      total_send_time += get_elapsed(t7, t8) * 1000000;
      fprintf(stdout, "proc_time=%.2f\n", get_elapsed(t5, t6) * 1000000);
  }
  //printf("Benchmark end...\n");
  fprintf(stdout, "config=%s data_size=%d c2n_time=%.2f k2u_time=%.2f memcpy_time=%.2f h2d_time=%.2f d2h_time=%.2f proc_time=%.2f send_time=%.2f\n",
            "cpu+fpga", data_size, 
            total_c2n_time / adjusted,
            total_k2u_time / adjusted,
            total_memcpy_time / adjusted,
            total_h2d_time / adjusted,
            total_d2h_time / adjusted,
            total_proc_time / adjusted,
            total_send_time / adjusted
            );

  //std::cout << "Execute: get_value\n";
  //auto get_run = get_krnl(device_buffer, out, array_size);
  //get_run.wait();

  //int reference[array_size] = {0};
  //for (int i = 0; i < array_size; i++) {
  //  reference[i] = value_map[i];
  //}

  //out.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

  //if (std::memcmp(out_map, reference, array_size))
  //  throw std::runtime_error("Value read back does not match reference");
  //close(sock_fd);

  exit(0);
  return 0;
}
}
