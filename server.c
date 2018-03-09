// Copyright (C) 2018 David Helkowski

#include "server.h"
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
//#define __ms_u_long long
//#include <mstcpip.h>
//#include <winsock2.h>

#include "lumith.h"
#include "tiny.h"
#include "thread_data.h"
#include "picohttpparser/picohttpparser.h"
#include "postdata.h"

serverc::serverc() {
    this->port = 9999;
}

serverc::~serverc() {
    if( this->lumith ) delete this->lumith;
    delete this->conf;
}

void serverc::init() {
    init_thread_data();
    
    signal( SIGPIPE, SIG_IGN ); // Ignore SIGPIPE; otherwise browser cancel will kill process
}

void serverc::dolisten() {
    this->insocket = open_socket( port );
    if( this->insocket <= 0 ) {
        perror("ERROR");
        exit( this->insocket );
    }
    printf( "listening on %d, socket is %d\n", port, this->insocket );
}

void serverc::loopAccepts() {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof( clientAddr );
    int connection;
    while( 1 ) {
        connection = accept( this->insocket, ( struct sockaddr *) &clientAddr, &clientLen );
        process( connection, &clientAddr );
        close( connection );
    }
}

void serverc::forkAccepts() {
    for( int i = 0; i < 10; i++ ) {
        int pid = fork();
        if( !pid ) this->loopAccepts(); // child
        else if( pid > 0 ) printf( "child pid is %d\n", pid ); // parent
        else perror("fork");
    }
    this->loopAccepts();
}

void serverc::readConf( char *file ) {
    parserc parser;
    this->conf = parser.parsefile( file );
    this->lumith = new lumithc( this->conf );
    
    nodec *port_node;
    if( ( port_node = this->conf->getnode( (char *) "port") ) ) {
        char *portz = port_node->getval();
        port = atoi( portz );
    }
}

ssize_t writen( int fd, void *usrbuf, size_t n ) {
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = (char *) usrbuf;

    while( nleft > 0 ) {
        if( ( nwritten = write(fd, bufp, nleft) ) <= 0 ) {
            if (errno == EINTR) nwritten = 0; // interrupted by sig handler return and call write() again
            else return -1; // errorno set by write()
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}

char *serverc::read_all( int fd, int *size ) {
    int cursize = 500;
    int read_size = 500;
    int curpos = 0;
    int totsize = 0;
    char *buffer = new char[cursize];
    
    while( 1 ) {
        read_size = cursize - curpos;
        int read_cnt = read( fd, &buffer[ curpos ], read_size );
        if( read_cnt < 0 && errno != EINTR ) return 0;
        if( !read_cnt ) break;
        totsize += read_cnt;
        
        if( read_cnt == read_size ) {
            read_size = cursize;
            int newsize = cursize * 2;
            curpos = totsize;
            char *newbuffer = new char[ newsize ];
            memcpy( newbuffer, buffer, cursize );
            cursize = newsize;
            delete buffer;
            buffer = newbuffer;
            continue;
        }
        break;
    }
    *size = totsize;
    return buffer;
}

int serverc::open_socket( int port ) {
    int insocket;
    int optval=1;
    socklen_t optlen = sizeof(optval);
    
    struct sockaddr_in serverAddr;

    if( ( insocket = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) return -1;

    // Reuse Address; since multiple threads are listening on the same port
    if( setsockopt( insocket, SOL_SOCKET, SO_REUSEADDR, ( const void * ) &optval, sizeof(int) ) < 0 ) return -1;
    
    if( getsockopt( insocket, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen ) < 0 ) {
    }
    else {
        printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));
        optval = 1;
        setsockopt( insocket, SOL_SOCKET, SO_KEEPALIVE, ( const void * ) &optval, sizeof(int) );
    
        /*struct tcp_keepalive vals;
        memset(&vals, 0, sizeof(vals));
        vals.onoff             = 1;    // non-zero means "enable"
        vals.keepalivetime     = 50;   // milliseconds
        vals.keepaliveinterval = 100;  // milliseconds
        DWORD numBytesReturned = 0;  // not really used AFAICT
        int ret = WSAIoctl(insocket, SIO_KEEPALIVE_VALS, &vals, sizeof(vals), NULL, 0, &numBytesReturned, NULL, NULL);
        printf("WSAIoctl returned %s\n", (ret==0)?"SUCCESS":"ERROR");*/
    }
    
    #if defined(__GLIBC__) && !defined(__FreeBSD_kernel__)
        printf("Setting keepalive time\n");
        //if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) < 0) {
        //    __redisSetError(c,REDIS_ERR_OTHER,strerror(errno));
        //    return REDIS_ERR;
        //}
    
        optval = 5 / 3;
        if( val == 0 ) val = 1;
        setsockopt( insocket, IPPROTO_TCP, TCP_KEEPINTVL, &optval, sizeof(optval) );
    
        val = 3;
        setsockopt( insocket, IPPROTO_TCP, TCP_KEEPCNT, &optval, sizeof(optval) );
    #endif

    // Listen on all IPs
    memset( &serverAddr, 0, sizeof(serverAddr) );
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl( INADDR_ANY );
    serverAddr.sin_port = htons( (unsigned short) port );
    if( bind( insocket, ( const sockaddr *) &serverAddr, sizeof( serverAddr ) ) < 0 ) return -1;

    // Begin listening
    if( listen( insocket, 1024 ) < 0 ) return -1;
    return insocket;
}

request *serverc::parse_request( int fd ) {
    int reqSize = 0;
    char *rawReq = read_all( fd, &reqSize );
    if( !rawReq || !reqSize ) return 0;
    printf("Request:\n%.*s\n", reqSize, rawReq );
    
    const char *method;
    size_t method_len;
    char *path;
    size_t path_len;
    int minor_version;
    struct phr_header *headers = new struct phr_header[15];
    size_t num_headers = 15;
    
    /*int newLen = reqSize;
    for( int i=0;i<reqSize;i++ ) {
        if( rawReq[ i ] == 0x0d && rawReq[ i + 1 ] == 0x0a && rawReq[ i + 2 ] == 0x0d && rawReq[ i + 3 ]  == 0x0a ) {
            newLen = i;
            break;
        }
    }*/
    int pret = phr_parse_request( rawReq, reqSize, &method, &method_len, &path, &path_len, &minor_version, headers, &num_headers, 0);
    printf("Req size: %i - pret: %i\n", reqSize, pret );
    printf("Num headers: %i\n", (int) num_headers );
    
    char *boundary = 0;
    int boundary_len;
    int dataType = 0;
    int keepAlive = 0;
    char expectContinue = 0;
    for( int i=0;i<(int) num_headers;i++ ) {
        struct phr_header *header = &headers[i];
        //1234567890123
        //Content-Type
        if( header->name_len == 12 && !strncmp( header->name, "Content-Type", 12 ) ) {
            //printf("Content type header value len: %i\n", header->value_len );
            if( header->value_len > 29 && !strncmp( header->value, "multipart/form-data; boundary=", 29 ) ) {
                //012345678901234567890123456789
                //multipart/form-data; boundary=
                dataType = POST_MULTIPART;
                boundary = (char *) &( header->value[ 30 ] );
                boundary_len = header->value_len - 30;
                printf("Boundary:%.*s\n", boundary_len, boundary );
            }
            if( header->value_len == 33 && !strncmp( header->value, "application/x-www-form-urlencoded", 33 ) ) {
                //123456789012345678901234567890123
                //application/x-www-form-urlencoded
                dataType = POST_URLENC;
            }
            if( header->value_len == 16 && !strncmp( header->value, "application/json", 16 ) ) {
                //1234567890123456
                //application/json
                dataType = POST_RAW;
            }
        }
        if( header->name_len == 10 && !strncmp( header->name, "Connection", 10 ) ) {
            if( header->value_len == 10 && !strncmp( header->value, "keep-alive", 10 ) ) {
                keepAlive = 1;
            }
        }
        if( header->name_len == 6 && !strncmp( header->name, "Expect", 6) ) {
            //100-Continue
            //123456789012
            if( header->value_len == 12 && !strncmp( header->value, "100-Continue", 12 ) ) {
                expectContinue = 1;
            }
        }
    }
    request *req = new request( path, path_len );
    req->headerCnt = num_headers;
    req->headers = headers;
    
    char *postData = 0;
    int postLen = 0;
    if( expectContinue ) {
        //                    1234567890123456789012 3 4 5
        writen( fd, (void *) "HTTP/1.1 100 Continue\r\n\r\n", 25 );
        // TODO: Make this read timeout faster than normal to mitigate denial of service attempts
        postData = read_all( fd, &postLen );
    }
    else if( pret < reqSize ) {
        postData = &rawReq[ pret ];
        postLen = reqSize - pret;
    }
    
    if( postData ) {
        printf("Leftover size %i:\n%.*s\n", postLen, postLen, postData );
        
        postdatac *postdata = new postdatac;
        req->postdata = postdata;
        if( dataType == POST_MULTIPART ) {
            char *boundZ = new char[ boundary_len + 3 ];
            boundZ[0] = '-';
            boundZ[1] = '-';
            memcpy( &boundZ[2], boundary, boundary_len );
            boundZ[ boundary_len + 2 ] = 0;
            postdata->parseMultipart( boundZ, &rawReq[ pret ], reqSize - pret );
            delete boundZ;
        }
        else if( dataType == POST_URLENC ) {
            printf("Urldecode\n");
            postdata->parseUrlencoded( &rawReq[ pret ], reqSize - pret );
        }
        else if( dataType == POST_RAW ) {
            postdata->rawPost( &rawReq[ pret ], reqSize - pret );
        }
        
        /*struct phr_chunked_decoder decode_data;
        memset( &decode_data, 0, sizeof( struct phr_chunked_decoder ) );
        char *curpos = &rawReq[ pret ];
        char *end = &rawReq[ reqSize ];
        size_t parsed_size = 0;
        while( 1 ) {
            ssize_t size = phr_decode_chunked( &decode_data, curpos, &parsed_size );
            printf("Parsed size: %i After decode:\n%.*s\n", parsed_size, parsed_size, curpos );
            if( size == -2 ) {
                curpos += parsed_size;
                if( curpos >= end ) break;
                break;
                continue;
            }
            break;
        }*/
    }
    
    if( dataType ) req->isPost = 1;
    req->keepAlive = keepAlive;
    return req;
}

void serverc::process( int fd, struct sockaddr_in *clientAddr ) {
    while( 1 ) {
        printf( "accept request, fd is %d, pid is %d\n", fd, getpid() );
        request *req = parse_request( fd );
        if( !req ) {
            printf("Abort continue\n");
            break;
        }
        req->setClientAddr( clientAddr );
        if( !req ) return;
        
        this->lumith->serve( fd, req );
        req->log();
        
        // Currently keepAlive doesn't function on mingw
        if( req->keepAlive ) {
            delete req;
            printf("continuing\n");
            continue;
        }
        delete req;
        break;
    }
}