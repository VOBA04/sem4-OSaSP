#define _XOPEN_SOURCE 500

#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <malloc.h>
#include <stdint.h>
#include <semaphore.h>
#include <sys/types.h>
#include <time.h>

sem_t *items;
sem_t *free_space;

pid_t *consumers = NULL;
int consumers_cnt = 0;

pid_t *producers = NULL;
int producers_cnt = 0;

volatile sig_atomic_t run = true;

void term_handler_producer(int sig)
{
    run = false;
    int sval;
    sem_getvalue(free_space, &sval);
    if (sval <= 0)
        exit(0);
}

void term_handler_consumer(int sig)
{
    run = false;
    int sval;
    sem_getvalue(items, &sval);
    if (sval <= 0)
        exit(0);
}

struct Message
{
    uint8_t type;
    uint16_t hash;
    uint8_t size;
    uint8_t data[];
};

#define MESSAGE_MAX_SIZE (sizeof(struct Message) + 255)

int MessageSize(struct Message *message)
{
    return sizeof(*message) + message->size;
}

struct Message *readMessage(struct Buffer *buffer)
{
    struct Message *head = alloca(MESSAGE_MAX_SIZE);
    readBytes(buffer, sizeof(struct Message), (uint8_t *)head);
    readBytes(buffer, head->size, head->data);
    buffer->extracted++;
    printf("Extracted messages count: %d\n", (buffer->extracted));
    return memcpy(malloc(MessageSize(head)), head, MessageSize(head));
}

void sendMessage(struct Buffer *buffer, struct Message *message)
{
    sendBytes(buffer, MessageSize(message), (char *)message);
    buffer->added++;
    printf("Added messages count: %d\n", buffer->added);
}

uint16_t hashCode(int length, char bytes[])
{
    uint16_t res = 0;
    for (int i = 0; i < length; i++)
    {
        res ^= bytes[i];
    }
    return res;
}

struct Message *randomMessage()
{
    uint8_t size = rand() % 256;
    struct Message *message = malloc(sizeof(struct Message) + size);
    *message = (struct Message){
        .type = rand(),
        .hash = 0,
        .size = size,
    };
    for (int i = 0; i < size; i++)
        message->data[i] = rand();
    message->hash = hashCode(sizeof(struct Message) + size, (char *)message);
    return message;
}

void freeMessage(struct Message *message)
{
    free(message);
}

void producerProc(struct Buffer *buffer)
{
    while (run)
    {
        sem_wait(free_space);
        struct Message *message = randomMessage();
        sendMessage(buffer, message);
        printf(
            "--------------------------------------------------\n"
            "Producer %d sent message with type %hX and hash %hX\n"
            "--------------------------------------------------\n",
            getpid(),
            message->type,
            message->hash);
        free(message);
        sem_post(items);
        sleep(3);
    }
    exit(0);
}

void consumerProc(struct Buffer *buffer)
{
    while (run)
    {
        sem_wait(items);
        struct Message *message = readMessage(buffer);
        printf(
            "--------------------------------------------------\n"
            "Consumer %d got  message: type: %hX hash: %hX\n"
            "--------------------------------------------------\n",
            getpid(),
            message->type,
            message->hash);
        free(message);
        sem_post(free_space);
        sleep(3);
    }
    exit(0);
}

void newProducer(struct Buffer *buffer)
{
    pid_t producer = fork();
    if (producer == 0)
    {
        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_handler = term_handler_producer;
        sa.sa_flags = 0;
        sigaction(SIGTERM, &sa, 0);
        srand(time(0));
        producerProc(buffer);
    }
    else
    {
        printf("New producer is created PID: %d\n", producer);
        producers_cnt++;
        producers = realloc(producers, sizeof(pid_t) * producers_cnt);
        producers[producers_cnt - 1] = producer;
    }
}

void killProducer()
{
    if (producers_cnt == 0)
        return;
    producers_cnt--;
    printf("Producer PID: %d is killed\n", producers[producers_cnt]);
    kill(producers[producers_cnt], SIGTERM);
    waitpid(producers[producers_cnt], NULL, 0);
}

void killAllProducers()
{
    while (producers_cnt)
        killProducer();
    free(producers);
    producers = NULL;
}

void newConsumer(struct Buffer *buffer)
{
    pid_t consumer = fork();
    if (consumer == 0)
    {
        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_handler = term_handler_consumer;
        sa.sa_flags = 0;
        sigaction(SIGTERM, &sa, 0);
        consumerProc(buffer);
    }
    else
    {
        printf("New consumer is created PID: %d\n", consumer);
        consumers_cnt++;
        consumers = realloc(consumers, sizeof(pid_t) * consumers_cnt);
        consumers[consumers_cnt - 1] = consumer;
    }
}

void killConsumer()
{
    if (consumers_cnt == 0)
        return;
    consumers_cnt--;
    printf("Consumer PID: %d is killed\n", consumers[consumers_cnt]);
    kill(consumers[consumers_cnt], SIGTERM);
    waitpid(consumers[consumers_cnt], NULL, 0);
}

void killAllConsumers()
{
    while (consumers_cnt)
        killConsumer();
    free(consumers);
    consumers = NULL;
}

int main()
{
    sem_unlink("/items");
    sem_unlink("/free_space");
    items = sem_open("/items", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR, 0);
    free_space = sem_open("/free_space", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR, 5);
    int capacity = 1024;
    struct Buffer *buffer = createBuffer(smalloc(sizeof(struct Buffer) + capacity), capacity);
    char opt[256];
    printf("Start program\n");
    while (scanf("%s", opt))
    {
        if (!strcmp(opt, "p"))
            newProducer(buffer);
        if (!strcmp(opt, "kp"))
            killProducer();
        if (!strcmp(opt, "kap"))
            killAllProducers();
        if (!strcmp(opt, "c"))
            newConsumer(buffer);
        if (!strcmp(opt, "kc"))
            killConsumer();
        if (!strcmp(opt, "kac"))
            killAllConsumers();
        if (!strcmp(opt, "ka"))
        {
            killAllProducers();
            killAllConsumers();
        }
        if (!strcmp(opt, "s"))
        {
            printf("Added: %d Extracted: %d\n", buffer->added, buffer->extracted);
        }
        if (!strcmp(opt, "q"))
        {
            killAllProducers();
            killAllConsumers();
            break;
        }
    }
    sem_unlink("/items");
    sem_unlink("/free_space");
    freeDesctruct(buffer);
    return 0;
}