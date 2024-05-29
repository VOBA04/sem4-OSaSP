#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

int Server(int client, char buffer[], char *dir)
{
    bool cd_flag = false;
    write(client, buffer, strlen(buffer));
    if (!strcmp(buffer, "QUIT"))
    {
        return -1;
    }
    if (strstr(buffer, "CD"))
        cd_flag = true;
    int buffer_count = read(client, buffer, 4096);
    if (buffer_count <= 0)
        printf("Reading error: %s\n", strerror(errno));
    else
    {
        buffer[buffer_count] = '\0';
        if (!strcmp(buffer, "\n\n"))
            return -1;
        if (cd_flag)
        {
            if (!strcmp(buffer, "\n"))
                printf("Wrong dir\n");
            else
                strcpy(dir, buffer);
        }
        else
            printf("%s", buffer);
        cd_flag = false;
    }
    return 0;
}

int main()
{
    char buffer[4096];
    int buffer_count;
    char *dir = (char *)calloc(256, sizeof(char));
    dir[0] = '/';
    int client = socket(AF_INET, SOCK_STREAM, 0);
    if (client == -1)
    {
        printf("Socket error: %s\n", strerror(errno));
        return 0;
    }
    struct sockaddr_in sa =
        {
            .sin_family = AF_INET,
            .sin_addr.s_addr = inet_addr("127.0.0.1"),
            .sin_port = htons(8080)};
    if (connect(client, (struct sockaddr *)&sa, sizeof(sa)) == 1)
    {
        printf("Connect error: %s\n", strerror(errno));
        return 0;
    }
    if ((buffer_count = read(client, buffer, sizeof(buffer))) > 0)
    {
        if (!strcmp(buffer, "\n\n"))
            return 0;
        printf("%s", buffer);
    }
    bool run = true;
    while (run)
    {
        do
        {
            printf("%s > ", dir);
            fgets(buffer, 4096, stdin);
        } while (!strcmp(buffer, "\n"));
        buffer[strlen(buffer) - 1] = '\0';
        if (buffer[0] == '@')
        {
            char file_name[255];
            strcpy(file_name, buffer + 1);
            FILE *fp = fopen(file_name, "r");
            if (!fp)
            {
                printf("File open error: %s\n", strerror(errno));
                continue;
            }
            while (!feof(fp))
            {
                fgets(buffer, sizeof(buffer), fp);
                if (!strcmp(buffer, "/n"))
                    continue;
                char *pos;
                if ((pos = strstr(buffer, "\n")))
                    *pos = '\0';
                printf("%s > %s\n", dir, buffer);
                if (Server(client, buffer, dir) == -1)
                {
                    run = false;
                    break;
                }
            }
            fclose(fp);
        }
        else
        {
            if (Server(client, buffer, dir) == -1)
                run = false;
        }
    }
    close(client);
    return 0;
}