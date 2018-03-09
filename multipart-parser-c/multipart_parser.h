/*
Based on:
  https://github.com/iafonov/multipart-parser-c - commit 772639c 
  Copyright (C) 2012 Igor Afonov
  MIT License - http://www.opensource.org/licenses/mit-license.php
Sublicense/Modifications: ( the contents of this file )
  Copyright (C) 2017 David Helkowski
  CS-REF License - http://www.carbonstate.com/csref.htm
*/
#ifndef _multipart_parser_h
#define _multipart_parser_h

#include <stdlib.h>
#include <ctype.h>

typedef struct multipart_parser multipart_parser;
typedef struct multipart_parser_settings multipart_parser_settings;
typedef struct multipart_parser_state multipart_parser_state;

typedef int (*multipart_data_cb) (void*, const char *at, size_t length);

struct multipart_parser_settings {
  multipart_data_cb on_header_field;
  multipart_data_cb on_header_value;
  multipart_data_cb on_part_data;
};

struct multipart_parser {
    void * data;
    size_t index;
    size_t boundary_length;
    unsigned char state;
    const multipart_parser_settings* settings;
    char multipart_boundary[1];
};

multipart_parser* multipart_parser__new( const char *boundary, const multipart_parser_settings* settings );
size_t multipart_parser_execute( multipart_parser* p, const char *buf, size_t len);

#endif
