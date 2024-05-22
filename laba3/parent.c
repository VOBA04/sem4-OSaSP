#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "action.h"

typedef struct
{
    char *name;
    pid_t pid;
} child_t;

char *CHILD_PATH;
child_t *children = NULL;
int children_cnt = 0;
volatile sig_atomic_t out_pause = false;

void create_child()
{
    int child_id = children_cnt;
    char child_name[10];
    if (child_id > 99)
    {
        printf("The maximum number of child processes has been reached (100)\n");
        return;
    }
    if (access(CHILD_PATH, X_OK))
    {
        printf("Child path not found or file is not executable\n");
        return;
    }
    snprintf(child_name, sizeof(child_name), "child_%02i", child_id);
    child_id++;
    pid_t pid = fork();
    if (pid == -1)
    {
        printf("Fork error: %s\n", strerror(errno));
        return;
    }
    if (pid == 0)
    {
        char *args[] = {child_name, NULL};
        execv(CHILD_PATH, args);
    }
    else
    {
        children = realloc(children, sizeof(child_t) * (children_cnt + 1));
        children[children_cnt].name = strdup(child_name);
        children[children_cnt].pid = pid;
        children_cnt++;
    }
}

void kill_child(child_t child)
{
    printf("Killing %s with PID: %i\n", child.name, child.pid);
    kill(child.pid, SIGKILL);
    waitpid(child.pid, NULL, 0);
    free(child.name);
}

void kill_last_child()
{
    if (children_cnt == 0)
    {
        printf("There are no children\n");
        return;
    }
    children_cnt--;
    kill_child(children[children_cnt]);
    children = realloc(children, sizeof(child_t) * children_cnt);
}

void kill_children()
{
    for (int i = 0; i < children_cnt; i++)
        kill_child(children[i]);
    free(children);
    children = NULL;
    children_cnt = 0;
}

void sigint_handler(int sig)
{
    (void)sig;
    kill_children();
    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}

void child_signal(int child_id, int action)
{
    union sigval val;
    val.sival_int = action;
    if (child_id < 0)
    {
        for (int i = 0; i < children_cnt; i++)
            sigqueue(children[i].pid, SIGRTMIN, val);
    }
    else if (child_id >= children_cnt)
    {
        printf("There is no child with id: %i\n", child_id);
    }
    else
    {
        sigqueue(children[child_id].pid, SIGRTMIN, val);
    }
}

void change_terminal(int term_n)
{
    char term_path[13];
    snprintf(term_path, sizeof(term_path), "/dev/pts/%i", term_n);
    if (access(term_path, F_OK))
    {
        printf("There is no terminal with id: %i\n", term_n);
        return;
    }
    child_signal(-1, CHANGE_TERMINAL + (term_n << 8));
}

void out_children()
{
    printf("-----------------------------------------\n");
    printf("Parent PID: %d\n", getpid());
    for (int i = 0; i < children_cnt; i++)
        printf("%s PID: %d\n", children[i].name, children[i].pid);
    printf("-----------------------------------------\n");
}

int parse_number()
{
    int child_id = 0;
    bool is_num = false;
    while (true)
    {
        char c = getchar();
        if (isspace(c) || c == EOF)
            break;
        is_num = true;
        child_id = child_id * 10 + (c - '0');
    }
    return is_num ? child_id : -1;
}

void alarm_handler(int sig)
{
    (void)sig;
    if (out_pause)
    {
        out_pause = false;
        child_signal(-1, ALLOW_OUT);
    }
    signal(SIGALRM, alarm_handler);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Wrong arguments\n");
        return 1;
    }
    CHILD_PATH = argv[1];
    if (access(CHILD_PATH, X_OK))
    {
        printf("Child path not found or file is not executable\n");
        return 1;
    }
    signal(SIGINT, sigint_handler);
    char c;
    while ((c = getchar()) != EOF)
    {
        switch (c)
        {
        case '+':
            create_child();
            break;
        case '-':
            kill_last_child();
            break;
        case 'l':
            out_children();
            break;
        case 'k':
            kill_children();
            break;
        case 's':
            child_signal(parse_number(), DISALLOW_OUT);
            break;
        case 'g':
            out_pause = false;
            child_signal(parse_number(), ALLOW_OUT);
            break;
        case 'p':
            out_pause = true;
            child_signal(-1, DISALLOW_OUT);
            child_signal(parse_number(), PRINT_STATS);
            struct sigaction alarm_act;
            alarm_act.sa_handler = alarm_handler;
            alarm_act.sa_flags = SA_RESTART;
            sigaction(SIGALRM, &alarm_act, NULL);
            alarm(5);
            break;
        case 't':
            change_terminal(parse_number());
            break;
        case 'q':
            kill_children();
            return 0;
        case '\n':
            continue;
        default:
            printf("Unknown option: %c\n", c);
            break;
        }
    }
    if (children_cnt)
    {
        kill_children();
    }
    return 0;
}