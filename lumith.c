// Copyright (C) 2018 David Helkowski

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "lumith.h"

#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include <nanomsg/pipeline.h>
#include "str_buffer.h"
#include "thread_data.h"
#include "base64.h"
#include "xmlbare/parser.h"
#include "static.h"
#include "tiny.h"
#include "request.h"
#include "response.h"
#include <pthread.h>
#include "postdata.h"

lumithc::lumithc( nodec *conf_in ) {
    conf = conf_in;
    
    nodec *in_node;
    if( ( in_node = conf->getnode( (char *) "socket_in") ) ) {
        printf("Socket in: %.*s\n", in_node->vallen, in_node->value );
        socket_address_in = in_node->getval();
    }
    else socket_address_in = 0;
    
    nodec *out_node;
    if( ( out_node = conf->getnode( (char *) "socket_out") ) ) {
        printf("Socket out: %.*s\n", out_node->vallen, out_node->value );
        socket_address_out = out_node->getval();
    }
    else socket_address_out = 0;
    
    nodec *static_node;
    if( ( static_node = conf->getnode( (char *) "static") ) ) {
        this->statics = new staticsc( static_node );
    }
    else this->statics = 0;
    
    pthread_mutex_init( &mutex, NULL );
    set_mutex( &mutex );
}

lumithc::~lumithc() {
    delete conf;
    pthread_mutex_destroy( &mutex );
    if( this->statics ) delete this->statics;
}

void lumithc::serve( int out_fd, request *req ) {
    responsec response;
    if( req->keepAlive ) response.keepAlive = 1;
    if( this->statics ) {
        mapped_folderc *folder = this->statics->ismapped( req->path );
        if( folder ) {
            handle_static( &response, req, folder );
            response.send( out_fd );
            return;
        }
    }
    
    handle_req( &response, req );
    response.send( out_fd );
}

void lumithc::handle_static( responsec *response, request *req, mapped_folderc *folder ) {
    char *local = folder->local;
    char *file = req->path->getfile();
    int localLen = strlen( local );
    int fileLen = strlen( file );
    
    // Create a full filename reference to the file to serve
    // TODO: Check for .. and spit out error if present
    char *full = new char[ localLen + fileLen + 1 ];
    memcpy( full, local, localLen );
    full[ localLen ] = '/';
    memcpy( &full[ localLen + 1 ], file, fileLen + 1 ); // copy final ending NULL also
    
    //str_appendZ( response->body, "Would serve file: " );
    //str_appendZ( response->body, full );
    
    char *buffer = 0;
    FILE *fh = fopen( full, "rb" );
    delete full;
    
    if( !fh ) {
        str_appendZ( response->body, "Could not find file: " );
        str_appendZ( response->body, full );
        return;
    }
    
    fseek( fh, 0, SEEK_END );
    long len = ftell( fh );
    fseek( fh, 0, SEEK_SET );
    buffer = new char[ len ];
    if( !buffer ) return;
    fread( buffer, 1, len, fh );
    fclose( fh );
    str_append( response->body, buffer, len );
    
    cookiesc *cookieset = &response->cookies;
    cookieType *cookie = cookieset->addCookie( (char *) "test", (char *) "x", 0, 0 );
    cookieset->cookieExpireSeconds( cookie, 100 );
}

int lumithc::handle_req( responsec *response, request *req ) {
    char strOut[BUFLEN];
    
    unsigned int thread_id = (unsigned int) getpid();
    
    thread_data *data = get_thread_data(thread_id);
    int request_num = data->request_num++;
    
    if( !data->socket_in_done ) {
        sprintf( strOut, socket_address_in, thread_id );
        data->socket_in = nn_socket(AF_SP, NN_PULL);
        int bind_res = nn_bind(data->socket_in, strOut);
        if( bind_res < 0 ) {
            int err = errno;
            char *errStr = decode_err( err );
            sprintf( strOut, "failed to bind: %s\n",errStr );
            printf( strOut );
            free( errStr );
            if( err != EADDRINUSE ) return 0;
        }
        data->socket_in_done = 1;
        int rcv_timeout = 2000;
        nn_setsockopt( data->socket_in, NN_SOL_SOCKET, NN_RCVTIMEO, &rcv_timeout, sizeof(rcv_timeout) );
    }
    
    if( !data->socket_out_done ) {
        data->socket_out = nn_socket(AF_SP, NN_PUSH);
        int connect_res = nn_connect(data->socket_out, socket_address_out);
        if( connect_res < 0 ) {
            printf("fail to connect\n");
            return 0;
        }
        data->socket_out_done = 1;
        int snd_timeout = 200;
        nn_setsockopt( data->socket_out, NN_SOL_SOCKET, NN_SNDTIMEO, &snd_timeout, sizeof(snd_timeout) );
    }
    
    int socket_in = data->socket_in;
    int socket_out = data->socket_out;
    
    str_buffer *xmlbuffer;
    request_to_xml( &xmlbuffer, thread_id, req, request_num );
    char *xml = xmlbuffer->data;
    
    int xmlLen = xmlbuffer->pos - xmlbuffer->data + 1;
    printf("Sending:\n%.*s\n", xmlLen, xml ); 
    int sent_bytes = nn_send(socket_out, xml, xmlLen, 0 );
    str_buffer__delete( xmlbuffer );
    if( !sent_bytes ) {
        printf("Fail to send\n");
        return 0;
    }
    usleep(200);
    
    while( 1 ) {
        char *buf = NULL;
        int bytes = nn_recv(socket_in, &buf, NN_MSG, 0);
        if( bytes < 0 ) {
           int err = errno;
           char *errStr = decode_err( err );
           
           char ptrStr[20];
           sprintf(ptrStr, "%p", (void *)data );//PRIx64
           
           char *body;
           int bodyLen = asprintf( &body, "failed to receive: %s - threadid:%u - data:%s - entries:%i\n",errStr,thread_id,ptrStr,0);//thread_entry_cnt );
           str_append( response->body, body, bodyLen );
           free( body );
           
           free( errStr );
           return 0;
        }
        else {
            //printf("Bytes: %i, Buffer:%s\n", bytes, buf );
            //char *dup = strndup( (char *) buf, bytes );
            parserc parser;
            nodec *root = parser.parse( buf );
            response->responseRoot = root;
            if( !root ) {
                char *body;
                int bodyLen = asprintf( &body, "Could not parse returned nano results" );
                str_append( response->body, body, bodyLen );
                free( body );
                
                return 0;
            }
            nodec *rn_node = root->getnode( (char *) "rn");
            int request_num_verify = -1;
            if( rn_node ) {
                char *nullStr = strndup( rn_node->value, rn_node->vallen );
                request_num_verify = atoi( nullStr );        
                free( nullStr );
            }
            if( request_num_verify < request_num ) {
                nn_freemsg( buf );
                continue;
            }
            
            int res = process_results( req, response, root, bytes, request_num );
            response->responseRaw = buf;
            //nn_freemsg( buf );
            return res;
        }
        break;
    }
    return 0;// should never reach here
}

int lumithc::process_results( request *req, responsec *response, nodec *root, int resLen, int request_num ) {
    nodec *redirect = root->getnode( (char *) "redirect");
    
    arrc *cookies = root->getnodes( (char *) "cookie");
    if( cookies ) {
        cookiesc *cookieset = &response->cookies;
        for( int i=0;i<cookies->count;i++ ) {
            nodec *cookieNode = ( nodec * ) cookies->items[ i ];
                              
            char *name = NULL;
            attc *nameNode = cookieNode->getatt( (char *) "key");
            if( nameNode ) name = nameNode->getval();
            
            char *value = NULL;
            attc *valNode = cookieNode->getatt( (char *) "val");
            if( valNode ) value = valNode->getval();
            
            char *path = (char *) "/";
            nodec *pathNode = cookieNode->getnode( (char *) "path");
            if( pathNode ) path = pathNode->getval();
            
            char *expires = NULL;
            nodec *expNode = cookieNode->getnode( (char *) "expires");
            
            cookieType *cookie = cookieset->addCookie( name, value, path, expires );
            if( expNode ) {
                const char *zStr = expNode->getval();
                int offsetSeconds = atoi( zStr );
                cookieset->cookieExpireSeconds( cookie, offsetSeconds );
            }
        }
    }
        
    if( redirect ) response->redirect = redirect->getval();
    nodec *content_type = root->getnode( (char *) "content_type");
    if( content_type ) response->contentType = content_type->getval();

    nodec *binary = root->getnode( (char *) "binary" );
    nodec *body = root->getnode( (char *) "body" );
    if( body ) {
        if( binary ) {
            char *nullStr = strndup( binary->value, binary->vallen );
            size_t binlen = atoi( nullStr );        
            free( nullStr );
                
            size_t len = binlen;
            unsigned char *data = new unsigned char[ binlen ];
            int result = Base64Decode( body->value, body->vallen, data, &len );
            if( result || len != binlen ) str_appendZ( response->body, "error" );
            else str_append( response->body, (char *) data, len );
            delete data;
        }
        else str_append( response->body, body->value, body->vallen );
    }
    return 0;
}

char *lumithc::decode_err( int err ) {
    char *buf;
    if( err == ENOENT ) { return strdup("ENOENT"); }
    if( err == EBADF ) { return strdup("EBADF"); }
    if( err == EMFILE ) { return strdup("EMFILE"); }
    if( err == EINVAL ) { return strdup("EINVAL"); }
    if( err == ENAMETOOLONG ) { return strdup("ENAMETOOLONG"); }
    if( err == EPROTONOSUPPORT ) { return strdup("EPROTONOSUPPORT"); }
    if( err == EADDRNOTAVAIL ) { return strdup("EADDRNOTAVAIL"); }
    if( err == ENODEV ) { return strdup("ENODEV"); }
    if( err == EADDRINUSE ) { return strdup("EADDRINUSE"); }
    if( err == ETERM ) { return strdup("ETERM"); }
    if( err == ENOTSUP ) { return strdup("ENOTSUP"); }
    if( err == EFSM ) { return strdup("EFSM"); }
    if( err == EAGAIN ) { return strdup("EAGAIN"); }
    if( err == EINTR ) { return strdup("EINTR"); }
    if( err == ETIMEDOUT ) { return strdup("ETIMEDOUT"); }
    buf = (char *) malloc(100);
    sprintf(buf,"Err number: %i", err );
    return buf;
}

int lumithc::request_to_xml( str_buffer **bufferOut, int thread_id, request *req, int request_num ) {
    // Write the thread id
    char tmp[100];
    snprintf( tmp, 100, "%u,", thread_id );
    
    str_buffer *buffer = str_buffer__new();
    *bufferOut = buffer;
    str_appendZ( buffer, tmp );
    
    req->headers_to_xml( buffer );
    
    if( req->isPost ) {
        postdatac *postdata = req->postdata;
        postdata->toxml( buffer );
    }
    
    snprintf( tmp, 100, "<rn>%i</rn>",request_num);
    str_appendZ( buffer, tmp );
    
    snprintf( tmp, 100, "<method>%s</method>", "GET" );//r->method );
    str_appendZ( buffer, tmp );
    
    //apr_uri_t uri = r->parsed_uri;
    str_append( buffer, "<uri path='", sizeof("<uri path='")-1 );
    //val_append( buffer, uri.path );
    val_append( buffer, req->getpath() );
    
    str_append( buffer, "' />", sizeof("' />")-1 );
    /*str_append( buffer, "' q='", sizeof("' q='")-1 );
    val_append_unescape( buffer, uri.query );
    str_append( buffer, "'/>", sizeof("'/>")-1 );*/
    
    //snprintf( tmp, 100, "<user ip='%s'/>", r->useragent_ip );
    
    // Return the length of the created string
    int len = buffer->pos - buffer->data;//pos - str;
    return len;
}