#include "common.h"

const char *server_ip = "10.0.0.1";
const char *client_ip = "10.0.0.2";

void warmup (int *fd, struct sockaddr_in *host)
{
    int msg_len         = 1024;
    char *msg           = (char *) malloc(msg_len);
    char *r_msg         = (char *) malloc(msg_len);
    socklen_t host_len  = sizeof(*host);

    strcpy(msg, "ready");

    printf("[client] warmup begin...\n");
    for (int i = 0; i < 10000; i++) {
        int sentlen = sendto(*fd, msg, msg_len, 0, (struct sockaddr *) host, host_len);
        if (sentlen <= 0)
            perror("sent\n");

        int recvlen = recvfrom(*fd, r_msg, msg_len, 0, (struct sockaddr *) host, 
                &host_len);
        if (recvlen <= 0)
            perror("recv\n");

        if (strcmp(r_msg, "done") != 0)
            fprintf(stderr, "msg=%s\n", r_msg);
    }
    printf("[client] warmup end...\n");
}

extern "C" {
int main(int argc, char *argv[])
{
    struct proc_config *config = parse_config(argc, argv);

    int key_count       = 100000;
    int adjusted        = key_count;
    size_t value_size   = config->value_size;
    int i = 0;

    char **many_values  = (char **) malloc(key_count * sizeof(char*));
    for(i = 0; i < key_count; i++)
    {
        many_values[i]  = (char *) malloc(value_size); 
        snprintf(many_values[i], 14, "value-%d:", i);
        gen_str(many_values[i], value_size);
    }

    fprintf(stderr, "%d Key is ready\n", key_count);

    int sock_fd = -1;
    struct sockaddr_in server;
    socklen_t server_len = sizeof(server);
    char *value = (char *) malloc(value_size);
    double total_pkt = 0.0;
    struct timespec pkt_begin, pkt_end;
    int sentlen = 0, recvlen = 0;

    struct msghdr *r_msg = create_msg(value_size, &server, server_len);
    struct msghdr *s_msg = create_msg(value_size, &server, server_len);

    struct timespec p1, p2, p3, rx_time;
    double total_k2u_time   = 0.0;
    double total_c2n_time   = 0.0;
    double total_send_time  = 0.0;

    config_socket(&sock_fd, SOCK_DGRAM, &server, server_ip, 9999, 0);
    //warmup(&sock_fd, &server);

    pkt_begin = snap_time();
    for (int i = 0; i < key_count; i++) {
        p1 = snap_time();
        sendto(sock_fd, many_values[i], value_size, 0, (struct sockaddr *) &server,
                server_len);
        p2 = snap_time();
        recvmsg(sock_fd, r_msg, 0);
        p3 = snap_time();
        rx_time = handle_time(r_msg, 1);

        total_send_time += get_elapsed(p1, p2) * 1000000;
        total_k2u_time  += get_elapsed(rx_time, p3) * 1000000;
        total_c2n_time  += (get_elapsed(p2, p3) - get_elapsed(rx_time, p3)) * 1000000;
    }
    pkt_end = snap_time();
    double e2e_time = get_elapsed(pkt_begin, pkt_end);
    printf("config=%s data_size=%ld elapsed=%.2fs avg_elapsed=%.2f send_time=%.2f c2n_time=%.2f k2u_time=%.2f\n",
            "client", value_size, e2e_time, e2e_time * 1000000 / adjusted,
            total_send_time / adjusted,
            total_c2n_time / adjusted,
            total_k2u_time / adjusted);

    free(many_values);
    free(value);
    close(sock_fd);

    return 0;
}
}
