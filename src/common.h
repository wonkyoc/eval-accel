#ifndef COMMON_H
#define COMMON_H

#include <asm-generic/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <linux/errqueue.h>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <net/if.h>

struct proc_config {
    int value_size;
};

/// @brief A wrapper for getting the current time.
/// @returns The current time.
static struct timespec snap_time()
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return t;
}

/// @brief Calculates the time difference between two struct timespecs
/// @param t1 The first time.
/// @param t2 The second time.
/// @returns The difference between the two times.
static double get_elapsed(struct timespec t1, struct timespec t2)
{
    double ft1 = t1.tv_sec + ((double)t1.tv_nsec / 1000000000.0);
    double ft2 = t2.tv_sec + ((double)t2.tv_nsec / 1000000000.0);
    return ft2 - ft1;
}


struct timespec handle_time(struct msghdr *msg, int is_hw);
struct msghdr * create_msg(int value_size, struct sockaddr_in *addr, 
        socklen_t addr_len);
void config_socket(int *fd, int sock_type, struct sockaddr_in *addr, 
        const char *ip, int port, int is_server);
void gen_str(char *str, int value_len);

struct proc_config *parse_config(int argc, char *argv[]);



#endif
