// Copyright (C) 2018 David Helkowski

#ifndef __RESPONSE_H
#define __RESPONSE_H

#include"str_buffer.h"
#include"xmlbare/parser.h"
#include"cookies.h"
class responsec {
    public:
    cookiesc cookies;
    char keepAlive;
    str_buffer *body;
    char *responseRaw;
    nodec *responseRoot;
    char *contentType;
    char *redirect;
    responsec();
    ~responsec();
    void send( int out_fd );
};

#endif