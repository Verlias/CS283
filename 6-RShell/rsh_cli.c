#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>

#include "dshlib.h"
#include "rshlib.h"

/*
 * exec_remote_cmd_loop(server_ip, port)
 */
int exec_remote_cmd_loop(char *address, int port) {
    int client_sock = start_client(address, port);
    if (client_sock < 0) {
        return ERR_RDSH_CLIENT;
    }

    char cmd_buffer[SH_CMD_MAX];
    char response_buffer[RDSH_COMM_BUFF_SZ];

    while (1) {
        printf("%s", SH_PROMPT);
        if (fgets(cmd_buffer, sizeof(cmd_buffer), stdin) == NULL) {
            printf("\n");
            break;
        }

        cmd_buffer[strcspn(cmd_buffer, "\n")] = '\0';

        // Handle exit command
        if (strcmp(cmd_buffer, "exit") == 0) {
            printf("Exiting client...\n");
            break;
        }

        if (send(client_sock, cmd_buffer, strlen(cmd_buffer) + 1, 0) < 0) {
            perror("send");
            break;
        }

        ssize_t bytes_received;
        do {
            bytes_received = recv(client_sock, response_buffer, RDSH_COMM_BUFF_SZ - 1, 0);
            if (bytes_received < 0) {
                perror("recv");
                break;
            }
            response_buffer[bytes_received] = '\0';
            printf("%s", response_buffer);
        } while (bytes_received > 0 && response_buffer[bytes_received - 1] != RDSH_EOF_CHAR);

        if (bytes_received < 0) {
            break;
        }
    }

    close(client_sock);
    return OK;
}


/*
 * start_client(server_ip, port)
 */
int start_client(char *server_ip, int port) {
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) {
        perror("socket");
        return ERR_RDSH_CLIENT;
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port)
    };

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(client_sock);
        return ERR_RDSH_CLIENT;
    }

    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(client_sock);
        return ERR_RDSH_CLIENT;
    }

    return client_sock;
}


/*
 * client_cleanup(int cli_socket, char *cmd_buff, char *rsp_buff, int rc)
 */
int client_cleanup(int cli_socket, char *cmd_buff, char *rsp_buff, int rc)
{
    if (cli_socket > 0)
    {
        close(cli_socket);
    }

    free(cmd_buff);
    free(rsp_buff);

    return rc;
}