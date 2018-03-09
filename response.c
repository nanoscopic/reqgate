// Copyright (C) 2018 David Helkowski

#include"response.h"
#include"server.h" // for writen
#include<nanomsg/nn.h> // for nn_freemsg
#include<string.h>
#include"str_buffer.h"

// close() here somewhere
#include<unistd.h>

responsec::responsec() {
    //response_type *response = ( response_type * ) malloc( sizeof( response_type ) );
    this->body = str_buffer__new();
    this->contentType = ( char * ) "text/html";
    this->responseRoot = 0;
    this->responseRaw = 0;
    this->keepAlive = 0;
    this->redirect = 0;
}

responsec::~responsec() {
    str_buffer__delete( this->body );
    if( this->responseRoot ) delete this->responseRoot;
    if( this->responseRaw ) nn_freemsg( this->responseRaw );
}

void responsec::send( int out_fd ) {
    str_buffer *buffer = str_buffer__new();
    
    if( this->redirect ) {
        str_appendZ( buffer, "HTTP/1.1 302 MOVED_TEMPORARILY\r\nLocation: ");
        str_appendZ( buffer, this->redirect );
        str_appendZ( buffer, "\r\n" );
    }
    else str_appendZ( buffer, "HTTP/1.1 200 OK\r\n");
    
    str_appendZ( buffer, "Cache-Control: no-cache\r\n");
    
    //str_buffer *body = this->body;
    long bodyLen = body->pos - body->data + 1;
    
    // Currently keepAlive doesn't function on mingw 
    if( this->keepAlive ) {
        str_appendZ( buffer, "Connection: Keep-Alive\r\n" );
        str_appendZ( buffer, "Keep-Alive: timeout=5, max=1000\r\n" );
    }
    cookies.output( buffer );
    
    if( this->redirect && !bodyLen ) {
    }
    else {
        char tmp[100];
        sprintf( tmp, "Content-length: %lu\r\n", bodyLen );
        str_appendZ( buffer, tmp );
        sprintf( tmp, "Content-type: %s\r\n\r\n", this->contentType );
        str_appendZ( buffer, tmp );
    }
        
    writen( out_fd, buffer->data, buffer->pos - buffer->data + 1 );
    
    if( this->redirect && !bodyLen ) {
    }
    else {
        writen( out_fd, body->data, bodyLen );
    }
    
    str_buffer__delete( buffer );
    
    //close( out_fd );
}
