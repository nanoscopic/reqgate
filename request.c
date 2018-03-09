// Copyright (C) 2018 David Helkowski

#include "request.h"
#include "urlpath.h"
#include <arpa/inet.h> /* inet_ntoa */

request::request( char *path, size_t pathlen ) {
    this->path = new urlpathc( path, pathlen );
    this->status = 200;
    this->keepAlive = 0;
    this->isPost = 0;
    this->headers = 0;
    this->headerCnt = 0;
}
request::~request() {
    if( this->headers ) delete this->headers;
}
char *request::getpath() {
    return this->path->str;
}
void request::setClientAddr( struct sockaddr_in *clientAddr ) {
    this->clientAddr = clientAddr;
    // inet_ntoa(c_addr->sin_addr), ntohs(c_addr->sin_port)
}
void request::log() {
    printf( "%s:%d %d - %s\n", inet_ntoa(this->clientAddr->sin_addr), ntohs(this->clientAddr->sin_port), this->status, this->getpath() );
}
void request::headers_to_xml( str_buffer *buffer ) {
}