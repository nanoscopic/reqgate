// Copyright (C) 2018 David Helkowski

#include "thread_data.h"
#include<stdlib.h>
#include<pthread.h>
thread_entry *thread_entries[20];
int thread_entry_cnt;

pthread_mutex_t *THmutex;

void set_mutex( pthread_mutex_t *mutexin ) {
    THmutex = mutexin;
}

thread_data *get_thread_data( unsigned int thread_id ) {
    for( int i=0;i<thread_entry_cnt;i++ ) {
        thread_entry *entry = thread_entries[i];
        if( entry->thread_id == thread_id ) return &entry->data;
    }
    // not found; add it
    pthread_mutex_lock( THmutex );
    //apr_thread_mutex_lock( mutex );
    thread_entry *new_entry = (thread_entry *) malloc( sizeof( thread_entry ) );
    
    new_entry->thread_id = thread_id;
    new_entry->data.socket_in_done = 0;
    new_entry->data.socket_out_done = 0;
    new_entry->data.request_num = 1;
    thread_entries[thread_entry_cnt] = new_entry;
    
    thread_entry_cnt++;
    pthread_mutex_unlock( THmutex );
    //apr_thread_mutex_unlock( mutex );
    
    return &new_entry->data;
}

void init_thread_data() {
    thread_entry_cnt = 0;
}