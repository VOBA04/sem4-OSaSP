#define _XOPEN_SOURCE 500
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <dirent.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

int client_count = 0;
int client_max_count;
char ROOT_DIR[] = "/mnt/D/University/cs/sem4";
sig_atomic_t run = 1;
int server;
int *clients_fd;
bool *clients_flags;

typedef struct
{
    int number;
    int fd;
} client_t;

void logg(time_t time, char message[])
{
    struct tm *now = localtime(&time);
    printf("%d.%02d.%02d-%02d:%02d:%02d --- %s",
           now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec, message);
}

void echo(client_t client, char *message)
{
    char logg_mess[4096];
    sprintf(logg_mess, "Client %d call ECHO with message: %s\n", client.number, message);
    logg(time(NULL), logg_mess);
    strcat(message, "\n");
    write(client.fd, message, strlen(message));
}

void info(client_t clien)
{
    char message[256];
    char logg_message[256];
    sprintf(logg_message, "Client %d get server info\n", clien.number);
    logg(time(NULL), logg_message);
    sprintf(message, "The server welcomes you!\nThe number of clients now: %d\nAvailable clients: %d\n",
            client_count, client_max_count - client_count);
    write(clien.fd, message, sizeof(message));
}

void cd(client_t client, char *path_to)
{
    char new_dir[256];
    char current_dir[256];
    char logg_message[4096];
    if (!strcmp(path_to, "/"))
    {
        chdir(ROOT_DIR);
        sprintf(logg_message, "Client %d chadge dir to root\n", client.number);
        logg(time(NULL), logg_message);
        write(client.fd, "/", strlen("/"));
    }
    else
    {
        getcwd(current_dir, 256);
        if (chdir(path_to) == -1)
        {
            sprintf(logg_message, "Client %d enter wrong path %s\n", client.number, path_to);
            logg(time(NULL), logg_message);
            write(client.fd, "\n", strlen("\n"));
        }
        else
        {
            getcwd(new_dir, 256);
            if (!strstr(new_dir, ROOT_DIR))
            {
                sprintf(logg_message, "Client %d try to out from root dir\n", client.number);
                chdir(current_dir);
                logg(time(NULL), logg_message);
                write(client.fd, "\n", strlen("\n"));
            }
            else
            {
                sprintf(logg_message, "Client %d change dir to %s\n", client.number, new_dir);
                logg(time(NULL), logg_message);
                if (!strcmp(new_dir, ROOT_DIR))
                    write(client.fd, "/", sizeof("/"));
                else
                {
                    char *pos = strstr(new_dir, ROOT_DIR);
                    pos += (strlen(ROOT_DIR) + 1);
                    write(client.fd, pos, strlen(pos));
                }
            }
        }
    }
}

void list(client_t client)
{
    char path[256];
    getcwd(path, 256);
    DIR *dir = opendir(path);
    struct dirent *dirent;
    char output[4096] = {0};
    char logg_message[4096];
    sprintf(logg_message, "Client %d get list of files in dir %s\n", client.number, path);
    logg(time(NULL), logg_message);
    while ((dirent = readdir(dir)) != NULL)
    {
        if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, ".."))
            continue;
        struct stat dirent_stat;
        getcwd(path, 256);
        strcat(path, "/");
        strcat(path, dirent->d_name);
        lstat(path, &dirent_stat);
        if (S_ISDIR(dirent_stat.st_mode))
        {
            strcat(output, dirent->d_name);
            strcat(output, "/");
        }
        else if (S_ISLNK(dirent_stat.st_mode))
        {
            strcat(output, dirent->d_name);
            char *pos = (char *)malloc(256);
            realpath(dirent->d_name, pos);
            struct stat link_stat;
            lstat(pos, &link_stat);
            if (S_ISLNK(link_stat.st_mode))
            {
                strcat(output, " -->> ");
            }
            else
            {
                strcat(output, " --> ");
            }
            strcat(output, pos + strlen(ROOT_DIR));
        }
        else
            strcat(output, dirent->d_name);
        strcat(output, "\n");
    }
    write(client.fd, output, strlen(output));
}

void *Client(void *arg)
{
    client_t client = *(client_t *)arg;
    char buffer[4096];
    int buffer_count;
    info(client);
    chdir(ROOT_DIR);
    while ((buffer_count = read(client.fd, buffer, sizeof(buffer))) > 0)
    {
        buffer[buffer_count] = '\0';
        char *p;
        if (!strncmp(buffer, "ECHO", 4))
        {
            p = strstr(buffer, "ECHO");
            echo(client, p + 5);
            continue;
        }
        if (!strncmp(buffer, "QUIT", 4))
        {
            char quit_mess[256];
            sprintf(quit_mess, "Client %d disconnected\n", client.number);
            logg(time(NULL), quit_mess);
            close(client.fd);
            client_count--;
            clients_flags[client.number] = false;
            break;
        }
        if (!strncmp(buffer, "INFO", 4))
        {
            info(client);
            continue;
        }
        if (!strncmp(buffer, "CD", 2))
        {
            p = strstr(buffer, "CD");
            cd(client, p + 3);
            continue;
        }
        if (!strncmp(buffer, "LIST", 4))
        {
            list(client);
            continue;
        }
        else
        {
            write(client.fd, "Incorrect command!\n", sizeof("Incorrect command!\n"));
            char error_mess[256];
            sprintf(error_mess, "Client %d enter incorrect message\n", client.number);
            logg(time(NULL), error_mess);
        }
    }
    pthread_exit(EXIT_SUCCESS);
}

void sigint_handler(int sig)
{
    (void)sig;
    run = 0;
    for (int i = 0; i < client_max_count; i++)
        if (clients_flags[i])
        {
            write(clients_fd[i], "\n\n", sizeof("\n\n"));
            close(clients_fd[i]);
        }
    printf("\nServer ended\n");
    close(server);
    exit(0);
}

int get_free_place()
{
    int i = 0;
    while (clients_flags[i] == 1 && i < client_max_count)
        i++;
    return i == client_max_count ? -1 : i;
}

int main(int argc, char *argv[])
{
    if (argc != 2 || (client_max_count = atoi(argv[1])) <= 0)
    {
        printf("Incorrect args!\n");
        return 0;
    }
    // client_max_count = 5;
    pthread_t *client_threads = (pthread_t *)malloc(client_max_count * sizeof(pthread_t));
    clients_fd = (int *)malloc(client_max_count * sizeof(int));
    clients_flags = (bool *)calloc(client_max_count, sizeof(bool));
    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1)
    {
        printf("Socket error: %s\n", strerror(errno));
        return 0;
    }
    struct sockaddr_in sa =
        {
            .sin_family = AF_INET,
            .sin_port = htons(8080),
        };
    if (bind(server, (struct sockaddr *)&sa, sizeof(sa)) == -1)
    {
        printf("Bind error: %s\n", strerror(errno));
        return 0;
    }
    if (listen(server, 5) == -1)
    {
        printf("Listen error: %s\n", strerror(errno));
        return 0;
    }
    signal(SIGINT, sigint_handler);
    printf("Server start\n"
           "Clients max count: %d\n",
           client_max_count);
    while (run)
    {
        struct sockaddr_in client_sa;
        socklen_t client_sa_len = sizeof(client_sa);
        int client = accept(server, (struct sockaddr *)&client_sa, &client_sa_len);
        if (client == -1)
        {
            printf("Accept error: %s\n", strerror(errno));
            continue;
        }
        else if (client_count < client_max_count)
        {
            int number = get_free_place();
            client_t *client_data = (client_t *)malloc(sizeof(client_t));
            client_data->fd = client;
            client_data->number = number;
            pthread_create(&client_threads[number], NULL, Client, (void *)client_data);
            client_count++;
            printf("Client %d connected\n", number - 1);
            clients_fd[number] = client;
            clients_flags[number] = true;
        }
        else
        {
            printf("There are more than %d clients\n", client_max_count);
            write(client, "\n\n", sizeof("\n\n"));
            close(client);
        }
    }
    return 0;
}
