#include "common.h"
#include <getopt.h>

static void print_time(const char *s, struct timespec* ts)
{
   printf("%s timestamp %ld.%lds\n", s,
          (uint64_t)ts->tv_sec, (uint64_t)ts->tv_nsec);

}


struct timespec handle_time(struct msghdr *msg, int is_hw) {
    struct cmsghdr *cmsg        = CMSG_FIRSTHDR(msg);
    struct scm_timestamping *ts = (struct scm_timestamping *) CMSG_DATA(cmsg);

  //print_time("System", &ts->ts[0]);
  //print_time("Transformed", &ts->ts[1]);
  //print_time("Raw", &ts->ts[1]);

    return ts->ts[0];
}

struct msghdr * create_msg(int value_size, struct sockaddr_in *addr, 
        socklen_t addr_len)
{
    char *value       = (char *) malloc(value_size);
    struct iovec *iov = (struct iovec *) malloc(sizeof(struct iovec));
    struct msghdr *m  = (struct msghdr *) malloc(sizeof(struct msghdr));
    char *ctrl        = (char *) malloc(64);

    iov->iov_base       = value;
    iov->iov_len        = value_size; 

    m->msg_name         = addr;
    m->msg_namelen      = addr_len;
    m->msg_iov          = iov;
    m->msg_iovlen       = 1;
    m->msg_control      = ctrl;
    m->msg_controllen   = 64;

    return m;
}


void config_socket(int *fd, int sock_type, struct sockaddr_in *addr, 
        const char *ip, int port, int is_server)
{
    int timestamp = 
      SOF_TIMESTAMPING_SOFTWARE | SOF_TIMESTAMPING_RAW_HARDWARE |
      SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RX_SOFTWARE;
    if ((*fd = socket(AF_INET, sock_type, 0)) < 0)
        perror("socket\n");

    addr->sin_family         = AF_INET;
    addr->sin_addr.s_addr    = inet_addr(ip);
    addr->sin_port           = htons(port);

    if (is_server) {
        if (bind(*fd, (struct sockaddr *) addr, sizeof(struct sockaddr_in)) < 0)
            perror("bind");
    }

    setsockopt(*fd, SOL_SOCKET, SO_TIMESTAMPING, &timestamp, sizeof(timestamp));
}

void gen_str(char *str, int value_len)
{
    const char *base_padding  = "ABCDEFGHIJKLMONPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    int padding_len     = strlen(base_padding);
    int str_len         = strlen(str);
    int offset          = str_len;
    int i               = 0;

    while (offset < value_len - offset) {
        memcpy(str + offset, base_padding, padding_len);
        offset += padding_len;
    }

    while (offset < value_len) {
        str[offset++] = base_padding[i];
        i++;
        i = i % 52;
    }
    str[offset] = '\0';
}


static void usage() {
    fprintf(stderr, "./app --msg-size size\n");
}

struct proc_config *parse_config(int argc, char *argv[])
{
    struct proc_config *config = (struct proc_config *) malloc(sizeof(struct proc_config));
    config->value_size = 64;

    int opt = 0;

    static struct option options[] = {
        {"msg-size", required_argument, 0, 'm'},
    };

    while ((opt = getopt_long(argc, argv, "m:" , options, NULL)) != -1) {
        switch (opt) {
            case 'm':
                config->value_size = (int) atoi(optarg);
                break;
            default:
                usage();
                exit(1);
        }
    }

    return config;
}
