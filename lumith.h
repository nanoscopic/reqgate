// Copyright (C) 2018 David Helkowski

#ifndef __LUMITH_H
#define __LUMITH_H
#include"tiny.h"
#include"str_buffer.h"
#include"xmlbare/parser.h"
#include"request.h"
#include"response.h"
#include"static.h"

class lumithc {
    nodec *conf;
    const char *socket_address_in;
    const char *socket_address_out;
    pthread_mutex_t mutex;
    staticsc *statics;
    
    public:
    lumithc( nodec *conf_in );
    ~lumithc();
    
    void serve( int out_fd, request *req );
    
    int handle_req( responsec *response, request *req );
    void handle_static( responsec *response, request *req, mapped_folderc *folder );
    int process_results( request *req, responsec *response, nodec *root, int resLen, int request_num );
    
    char *decode_err( int err );
    int request_to_xml( str_buffer **bufferOut, int thread_id, request *req, int request_num );
};
#endif