#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <locale.h>
#include <wait.h>

#define MAX_STR_LENGTH 4096

int compare_strings_lc(const void *a, const void *b)
{
    return strcoll((char *)a, (char *)b);
}

int size_of_arr(char **arr)
{
    int n = 0;
    while (arr[n] != NULL)
    {
        n++;
    }
    return n;
}

void inc_str_num(char *str)
{
    if (str[1] == '9' && str[0] == '9')
    {
        str[0] = '0';
        str[1] = '0';
    }
    else if (str[1] == '9')
    {
        str[0]++;
        str[1] = '0';
    }
    else
    {
        str[1]++;
    }
}

char **file_to_str_mass(FILE *fp)
{
    char **mass = (char **)malloc(sizeof(char *));
    int size = 0;
    while (!feof(fp))
    {
        size++;
        mass = (char **)realloc(mass, size * sizeof(char *));
        mass[size - 1] = (char *)malloc(255);
        fgets(mass[size - 1], 255, fp);
        if (mass[size - 1][strlen(mass[size - 1]) - 1] == '\n')
            mass[size - 1][strlen(mass[size - 1]) - 1] = '\0';
    }
    mass = (char **)realloc(mass, (size + 1) * sizeof(char *));
    mass[size] = NULL;
    return mass;
}

void flush()
{
    while (getchar() != '\n')
        continue;
}

int keyscmp(char *key, char *envp)
{
    char *envp_key = malloc(MAX_STR_LENGTH);
    int i = 0;
    while (envp[i] != '=')
    {
        envp_key[i] = envp[i];
        i++;
    }
    return strcmp(key, envp_key);
}