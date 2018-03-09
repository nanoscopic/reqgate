// Copyright (C) 2018 David Helkowski

#ifndef __MULTIPART_H
#define __MULTIPART_H
#include "multipart-parser-c/multipart_parser.h"
#include "str_buffer.h"

struct postval_s {
    char *name;
    int namelen;
    char *value;
    int valuelen;
    struct postval_s *next;
};
typedef struct postval_s postvalType;

#define POST_URLENC 1
#define POST_MULTIPART 2
#define POST_RAW 3
class postdatac {
    multipart_parser_settings callbacks;
    multipart_parser* parser;
    static int partname_callback( void* p, const char *data, size_t length );
    static int partvalue_callback( void* p, const char *data, size_t length );
    static int data_callback( void* p, const char *data, size_t length );
    char isContent;
    char *rawPostData;
    int rawPostLen;
    char firstData = 0;
    public:
    postvalType *firstVal;
    postvalType *lastVal;
    char type;
    postdatac();
    void parseMultipart( char *boundary, char *data, int len );
    void parseUrlencoded( char *data, int len );
    void rawPost( char *data, int len );
    void onname( char *value, size_t len );
    void ondata( const char *value, size_t len );
    void toxml( str_buffer *buffer );
};
     
#endif