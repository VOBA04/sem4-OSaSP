#define _XOPEN_SOURCE 500 // for lstat

#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <errno.h>
#include <linux/limits.h>

void travel(char *current_dir, bool symlinks, bool dirs, bool files, char ***pathes, int *pathes_num)
{
    struct dirent *dirent;
    char path[PATH_MAX + 1];
    DIR *dir = opendir(current_dir);
    if (dir == NULL)
    {
        printf("Error opening %s: %s\n", current_dir, strerror(errno));
        return;
    }
    while ((dirent = readdir(dir)) != NULL)
    {
        struct stat dirent_stat;
        if ((strcmp(dirent->d_name, ".") == 0) || (strcmp(dirent->d_name, "..") == 0))
            continue;
        strcpy(path, current_dir);
        strcat(path, "/");
        strcat(path, dirent->d_name);
        if (lstat(path, &dirent_stat) == -1)
            printf("Error statting %s: %s\n", path, strerror(errno));
        if ((symlinks && S_ISLNK(dirent_stat.st_mode)) || (dirs && S_ISDIR(dirent_stat.st_mode)) || (files && S_ISREG(dirent_stat.st_mode)))
        {
            (*pathes_num)++;
            *pathes = realloc(*pathes, sizeof(char *) * (*pathes_num));
            (*pathes)[*pathes_num - 1] = malloc(PATH_MAX + 1);
            strcpy((*pathes)[*pathes_num - 1], path);
        }
        if (S_ISDIR(dirent_stat.st_mode))
            travel(path, symlinks, dirs, files, pathes, pathes_num);
    }
    closedir(dir);
}

int compare_strings_lc(const void *a, const void *b)
{
    return strcoll(a, b);
}

int main(int argc, char *argv[])
{
    int opt;
    char *path = ".";
    char **pathes;
    pathes = malloc(sizeof(char *));
    int pathes_num = 0;
    bool symlinks = false, dirs = false, files = false, sorting = false;
    while ((opt = getopt(argc, argv, "ldfs")) != -1)
    {
        switch (opt)
        {
        case 'l':
            symlinks = true;
            break;
        case 'd':
            dirs = true;
            break;
        case 'f':
            files = true;
            break;
        case 's':
            sorting = true;
            break;
        case '?':
            printf("Unknown option: %c\n", optopt);
            break;
        default:
            break;
        }
    }
    if (argc - optind > 1)
    {
        printf("Too many arguments\n");
        return 0;
    }
    if (symlinks == false && dirs == false && files == false)
    {
        symlinks = true;
        dirs = true;
        files = true;
    }
    if (optind < argc)
    {
        path = argv[optind];
    }
    travel(path, symlinks, dirs, files, &pathes, &pathes_num);
    if (sorting)
        qsort(pathes, pathes_num, sizeof(char *), compare_strings_lc);
    for (int i = 0; i < pathes_num; i++)
    {
        printf("%s\n", pathes[i]);
    }
    return 0;
}