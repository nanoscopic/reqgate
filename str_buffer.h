// Copyright (C) 2018 David Helkowski

#ifndef __STR_BUFFER__H
#define __STR_BUFFER__H

#define BUFLEN 5000

struct str_buffer_s {
    char *data;
    char *pos;
    char *maxpos;
    int maxlen;
};
typedef struct str_buffer_s str_buffer;

str_buffer *str_buffer__new();
void str_buffer__delete( str_buffer *buffer );
void str_buffer__extend( str_buffer *buffer, int extraNeeded );
char hex2char( char b1, char b2 );
void str_appendZ( str_buffer *buffer, const char *str );
void str_append( str_buffer *buffer, const char *str, int len );
void val_append( str_buffer *buffer, const char *val );
void val_append_unescape( str_buffer *buffer, const char *val );

#endif