// Copyright (C) 2018 David Helkowski

#ifndef __SERVER_H
#define __SERVER_H

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include "xmlbare/parser.h"
#include "request.h"
#include "lumith.h"

ssize_t writen( int fd, void *usrbuf, size_t n );

class serverc {
    int insocket;
    int port;
    nodec *conf;
    lumithc *lumith;
    
    char *read_all( int fd, int *size );
    int open_socket( int port );
    request *parse_request( int fd );
    void process( int fd, struct sockaddr_in *clientAddr );
    public:
    serverc();
    ~serverc();
    void init();
    void dolisten();
    void forkAccepts();
    void readConf( char *file );
    void loopAccepts();
};
#endif