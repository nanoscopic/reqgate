// Copyright (C) 2018 David Helkowski

#ifndef __THREAD_DATA__H
#define __THREAD_DATA__H

#include<pthread.h>

void init_thread_data();

struct thread_data_s {
    int socket_in;
    int socket_in_done;
    int socket_out;
    int socket_out_done;
    int request_num;
};
typedef struct thread_data_s thread_data;

struct thread_entry_s {
    unsigned int thread_id;
    thread_data data;
};
typedef struct thread_entry_s thread_entry;

thread_data *get_thread_data( unsigned int thread_id );
void set_mutex( pthread_mutex_t *mutexin );

#endif