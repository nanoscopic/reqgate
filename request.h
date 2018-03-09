// Copyright (C) 2018 David Helkowski

#ifndef __REQUEST_H
#define __REQUEST_H
#include<stdio.h>
#include"urlpath.h"
#include<netinet/in.h>
#include<netinet/tcp.h>
#include"postdata.h"
#include"picohttpparser/picohttpparser.h"

class request {
    struct sockaddr_in *clientAddr;
    // Range support
    /*off_t offset; 
    size_t end;*/
    public:
    struct phr_header *headers;
    int headerCnt;
    postdatac *postdata;
    char isPost;
    urlpathc *path;
    int status;
    int keepAlive;
    request( char *path, size_t pathlen );
    ~request();
    char *getpath();
    void setClientAddr( struct sockaddr_in *clientaddr );
    void log();
    void headers_to_xml( str_buffer *buffer );
};
#endif