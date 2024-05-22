#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include "action.h"

struct Pair
{
    int a;
    int b;
} pair = {0, 0};

struct Stats
{
    int a0b0;
    int a0b1;
    int a1b0;
    int a1b1;
} stats = {0, 0, 0, 0};

volatile sig_atomic_t allow_out = true;

char *CHILD_NAME;

void alarm_handler(int sig)
{
    (void)sig;
    if (pair.a == 0 && pair.b == 0)
        stats.a0b0++;
    else if (pair.a == 0 && pair.b == 1)
        stats.a0b1++;
    else if (pair.a == 1 && pair.b == 0)
        stats.a1b0++;
    else
        stats.a1b1++;
    signal(SIGALRM, alarm_handler);
    alarm(1);
}

void change_pair()
{
    usleep(80000);
    if (pair.a == 0)
    {
        pair.a = 1;
        usleep(50000);
        pair.b = 1;
    }
    else
    {
        pair.a = 0;
        usleep(60000);
        pair.b = 0;
    }
}

void print_stats(bool force)
{
    if (force || allow_out)
        printf("%s with PID %i\t00: %i\t01: %i\t10: %i\t11: %i\n",
               CHILD_NAME, getpid(), stats.a0b0, stats.a0b1, stats.a1b0, stats.a1b1);
}

void change_terminal(int term_n)
{
    char term_path[13];
    snprintf(term_path, sizeof(term_path), "/dev/pts/%i", term_n);
    int new_out = open(term_path, O_WRONLY);
    dup2(new_out, 1);
    close(new_out);
}

void parent_handler(int sig, siginfo_t *info, void *ctx)
{
    (void)sig;
    (void)ctx;
    int command = info->si_value.sival_int & 0xFF;
    switch (command)
    {
    case ALLOW_OUT:
        allow_out = true;
        break;
    case DISALLOW_OUT:
        allow_out = false;
        break;
    case PRINT_STATS:
        print_stats(true);
        break;
    case CHANGE_TERMINAL:
        change_terminal(info->si_value.sival_int >> 8);
        break;
    default:
        break;
    }
}

int main(int argc, char *argv[])
{
    (void)argc;
    CHILD_NAME = argv[0];
    if (prctl(PR_SET_NAME, CHILD_NAME) == -1)
    {
        printf("prctl error: %s\n", strerror(errno));
        exit(1);
    }
    printf("-----------------------------------------\n");
    printf("Child started\nName: %s\nPID: %d\nPPID: %d\n", CHILD_NAME, getpid(), getppid());
    printf("-----------------------------------------\n");
    signal(SIGALRM, alarm_handler);
    struct sigaction parent_req;
    parent_req.sa_sigaction = &parent_handler;
    parent_req.sa_flags = SA_RESTART | SA_SIGINFO;
    if (sigaction(SIGRTMIN, &parent_req, NULL) == -1)
    {
        printf("Sigaction error: %s\n", strerror(errno));
        exit(1);
    }
    alarm(1);
    int i = 0;
    while (true)
    {
        change_pair();
        i++;
        if (i > 10)
        {
            print_stats(false);
            i = 0;
        }
    }
}