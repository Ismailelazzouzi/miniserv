#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>

typedef struct
{
    int id, fd;
    char *msg;
} client;


typedef struct
{
    int socket, max;
    char wbuff[48];
    char rbuff[1025];
    client *cl[1024];
    fd_set all, re, wr;
} myServer;

myServer srv;

int ft_strlen(char *str)
{
    if (!str)
        return 0;
    return (strlen(str));
}

void error_msg(char *str)
{
    int i = 0;
    while (i < ft_strlen(str))
    {
        write(2, &str[i], 1);
        i++;
    }
    exit(1);
}

void send_all(int exfd, char *msg)
{
    for (int i = 0; i < srv.max; i++)
    {
        if (FD_ISSET(i, &srv.wr) && i != exfd)
            send(i, msg, ft_strlen(msg), 0);
    }
    
}

int main(int ac, char **av)
{
    if (ac != 2)
        error_msg("Wrong number of arguments\n");
    srv.socket = socket(AF_INET, SOCK_STREAM, 0);
    if (srv.socket < 0)
        error_msg("Fatal error\n");
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(atoi(av[1]));
    if (bind(srv.socket, (const struct sockaddr *)&addr, sizeof(addr)) || listen(srv.socket, 10))
        error_msg("Fatal error\n");
    srv.max = srv.socket + 1;
    FD_ZERO(&srv.all);
    FD_SET(srv.socket, &srv.all);
    static int nextId = 0;
    while (1)
    {
        srv.re = srv.wr = srv.all;
        if (select(srv.max ,&srv.re, &srv.wr, NULL, NULL) < 0)
            error_msg("error\n");
        for (int fd = 0; fd < srv.max; fd++)
        {
            if (!FD_ISSET(fd, &srv.re))
                continue;
            if (fd == srv.socket)
            {
                int conn = accept(srv.socket, NULL, NULL);
                if (conn <= 0)
                    error_msg("error\n");
                client *new = calloc(1, sizeof(client));
                new->fd = conn;
                new->id = nextId++;
                FD_SET(conn, &srv.all);
                for (int i = 0; i < 1024; i++)
                {
                    if (!srv.cl[i])
                    {
                        srv.cl[i] = new;
                        break;
                    }
                }
                srv.max = conn <= srv.max ? conn + 1 : srv.max;
                sprintf(srv.wbuff, "server: client %d just arrived\n", new->id);
                send_all(conn, srv.wbuff);
            }
            else
            {
                int rcvb = recv(fd, srv.rbuff, 1024, 0);
                if (rcvb <= 0)
                {
                    int id = -1;
                    for (int i = 0; i < 1024; i++)
                    {
                        if (srv.cl[i] && srv.cl[i]->fd == fd)
                        {
                            free(srv.cl[i]->msg);
                            close(fd);
                            FD_CLR(fd, &srv.all);
                            id = srv.cl[i]->id;
                            free(srv.cl[i]);
                            srv.cl[i] = NULL;
                            sprintf(srv.wbuff, "server: client %d just left\n", id);
                            send_all(fd, srv.wbuff);
                            break;
                        }
                    }
                }
                else
                {
                    // extract message
                    // broadcast message
                }
            }
        }
    }
    
    return 0;
}