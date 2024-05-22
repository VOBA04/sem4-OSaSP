#define _POSIX_C_SOURCE 200112L
#include "utils.h"

int main(int argc, char *argv[], char *envp[])
{
    printf("------------------------------------------------------------------\n");
    printf("Name: %s; PID: %i; PPID: %i\n\n", argv[0], getpid(), getppid());
    FILE *fp = fopen(argv[2], "r");
    char **env_keys = file_to_str_mass(fp);
    int i, j;
    fclose(fp);
    i = 0;
    switch (argv[1][0])
    {
    case '+':
    {
        i = 0;
        while (env_keys[i] != NULL)
        {
            printf("%i %s=", i + 1, env_keys[i]);
            char *env = getenv(env_keys[i]);
            if (env == NULL)
                printf("null\n");
            else
                printf("%s\n", env);
            i++;
        }
        break;
    }
    case '*':
    {
        i = 0;
        while (env_keys[i] != NULL)
        {
            j = 0;
            while (envp[j] != NULL)
            {
                if (keyscmp(env_keys[i], envp[j]) == 0)
                {
                    printf("%i %s\n", i + 1, envp[j]);
                    break;
                }
                j++;
            }
            i++;
        }
        break;
    }
    case '&':
    {
        extern char **environ;
        i = 0;
        while (env_keys[i] != NULL)
        {
            j = 0;
            while (environ[j] != NULL)
            {
                if (keyscmp(env_keys[i], envp[j]) == 0)
                {
                    printf("%i %s\n", i + 1, environ[j]);
                    break;
                }
                j++;
            }
            i++;
        }
        break;
    }
    default:
        break;
    }
    printf("------------------------------------------------------------------\n");
    exit(0);
}