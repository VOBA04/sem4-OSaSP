#define _XOPEN_SOURCE 500
#include "utils.h"

int main(int argc, char *argv[], char *envp[])
{
    char child_n[3] = "00\0";
    char c;
    char **child_argv;
    int envp_n = size_of_arr(envp);
    int i;
    char *hostname = malloc(MAX_STR_LENGTH);
    char *hostname_env = malloc(MAX_STR_LENGTH);
    char **envp_copy = malloc((envp_n + 1) * sizeof(char *));
    if (argc < 2)
    {
        printf("Enter path to the file with environment variables\n");
        return 0;
    }
    gethostname(hostname, MAX_STR_LENGTH);
    strcat(hostname_env, "HOSTNAME=");
    strcat(hostname_env, hostname);
    child_argv = malloc(4 * sizeof(char *));
    for (i = 0; i < 3; i++)
        child_argv[i] = malloc(MAX_STR_LENGTH);
    child_argv[3] = NULL;
    strcpy(child_argv[2], argv[1]);
    // strcpy(child_argv[2], "/mnt/D/University/cs/sem4/OSaSP/laba2/reduced_env");
    pid_t pid;
    qsort(envp, envp_n, sizeof(char *), compare_strings_lc);
    i = 0;
    while (envp[i] != NULL)
    {
        printf("%s\n", envp[i]);
        envp_copy[i] = malloc(MAX_STR_LENGTH);
        strcpy(envp_copy[i], envp[i]);
        i++;
    }
    envp_copy[i] = malloc(MAX_STR_LENGTH);
    strcpy(envp_copy[i], hostname_env);
    envp_copy[i + 1] = NULL;
    printf("\nEnter character: ");
    while ((c = getchar()) != 'q')
    {
        switch (c)
        {
        case '+':
        {
            pid = fork();
            if (pid == -1)
            {
                printf("Error: %s\n", strerror(errno));
                return 0;
            }
            if (pid == 0)
            {
                printf("Child process created\n");
                strcpy(child_argv[0], "child_");
                strcat(child_argv[0], child_n);
                strcpy(child_argv[1], "+");
                if (execve(getenv("CHILD_PATH"), child_argv, envp_copy) == -1)
                    printf("Error: %s\n", strerror(errno));
            }
            waitpid(pid, NULL, 0);
            inc_str_num(child_n);
            break;
        }
        case '*':
        {
            pid = fork();
            if (pid == -1)
            {
                printf("Error: %s\n", strerror(errno));
                return 0;
            }
            if (pid == 0)
            {
                printf("Child process created\n");
                strcpy(child_argv[0], "child_");
                strcat(child_argv[0], child_n);
                strcpy(child_argv[1], "*");
                i = 0;
                char *child_path = malloc(MAX_STR_LENGTH);
                while (envp[i] != 0)
                {
                    if (keyscmp("CHILD_PATH", envp[i]) == 0)
                    {
                        strcpy(child_path, envp[i] + 11);
                        break;
                    }
                    i++;
                }
                if (execve(child_path, child_argv, envp_copy) == -1)
                    printf("Error *: %s\n", strerror(errno));
            }
            waitpid(pid, NULL, 0);
            inc_str_num(child_n);
            break;
        }
        case '&':
        {
            pid = fork();
            if (pid == -1)
            {
                printf("Error: %s\n", strerror(errno));
                return 0;
            }
            if (pid == 0)
            {
                printf("Child process created\n");
                strcpy(child_argv[0], "child_");
                strcat(child_argv[0], child_n);
                strcpy(child_argv[1], "&");
                i = 0;
                extern char **environ;
                char *child_path = malloc(MAX_STR_LENGTH);
                while (environ[i] != 0)
                {
                    if (keyscmp("CHILD_PATH", environ[i]) == 0)
                    {
                        strcpy(child_path, environ[i] + 11);
                        break;
                    }
                    i++;
                }
                if (execve(child_path, child_argv, envp_copy) == -1)
                    printf("Error: %s\n", strerror(errno));
            }
            waitpid(pid, NULL, 0);
            inc_str_num(child_n);
            break;
        }
        default:
            printf("Wrong character\n");
            break;
        }
        flush();
        printf("\nEnter character: ");
    }
    return 0;
}