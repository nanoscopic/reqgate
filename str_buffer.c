// Copyright (C) 2018 David Helkowski

#include"str_buffer.h"
#include<stdlib.h>
#include<string.h>

str_buffer *str_buffer__new() {
    str_buffer *buffer = (str_buffer *) calloc( sizeof( str_buffer ), 1 );
    buffer->data = buffer->pos = (char *) malloc( BUFLEN );
    buffer->maxlen = BUFLEN;
    buffer->maxpos = buffer->pos + BUFLEN - 1;
    return buffer;
}
void str_buffer__delete( str_buffer *buffer ) {
    free( buffer->data );
    free( buffer );
}
void str_buffer__extend( str_buffer *buffer, int extraNeeded ) {
    int curmax = buffer->maxlen;
    int curoffset = buffer->pos - buffer->data;
    int extra = curmax;
    if( extra < ( extraNeeded + 50 ) ) {
        extra = extraNeeded + 100;
    }
    int newmax = curmax + extra;
    char *newdata = (char *) malloc( newmax );
    memcpy( newdata, buffer->data, buffer->pos - buffer->data + 1 );
    free( buffer->data );
    buffer->data = newdata;
    buffer->pos = buffer->data + curoffset;
    buffer->maxlen = newmax;
    buffer->maxpos = buffer->data + newmax - 1;
}

void str_appendZ( str_buffer *buffer, const char *str ) {
    int len = strlen( str );
    str_append( buffer, str, len );
}

void str_append( str_buffer *buffer, const char *str, int len ) {
    char *pos = buffer->pos;
    if( ( pos + len ) > buffer->maxpos ) str_buffer__extend( buffer, ( pos + len ) - buffer->maxpos );
    //if( len > maxlen ) return;
    memcpy( buffer->pos, str, len );
    buffer->pos = buffer->pos + len;
    (buffer->pos)[0] = 0x00;
}

void val_append( str_buffer *buffer, const char *val ) {
    if( !val ) return;
    char *pos = buffer->pos;
    
    int i = 0;
    while( 1 ) {
    //for( int i=0;1;i++ ) {
        if( ( pos + 2 ) >= buffer->maxpos ) {
            str_buffer__extend( buffer, 2 );
            pos = buffer->pos;
        }
        char let = val[i];
        if( !let ) {
            pos[0] = 0;
            break;
        }
        if(      let == '\\' ) { pos[0] = '\\'; pos[1] = '\\'; pos = pos + 2; }
        else if( let == '\'' ) { pos[0] = '\\'; pos[1] = '\''; pos = pos + 2; }
        else if( let == '"'  ) { pos[0] = '\\'; pos[1] = '"';  pos = pos + 2; }
        else {
            pos[0] = let;
            pos++;
        }
        i++;
    }
    buffer->pos = pos;
}
void val_append_unescape( str_buffer *buffer, const char *val ) {
    if( !val ) return;
    
    char *pos = buffer->pos;
    int i = 0;
    while(1) {
    //for( int i=0;1;i++ ) {
        if( ( pos + 2 ) >= buffer->maxpos ) {
            str_buffer__extend( buffer, 2 );
            pos = buffer->pos;
        }
        
        char let = val[i];
        if( !let ) {
            pos[0] = 0;
            break;
        }
        
        if( let == '%'  ) {
            //if( ( i + 2 ) < maxlen ) {
                char hex1 = val[i+1];
                char hex2 = val[i+2];
                let = hex2char( hex1, hex2 );
                i+=2;
            //}
        }
        
        if(      let == '\\' ) { pos[0] = '\\'; pos[1] = '\\'; pos = pos + 2; }
        else if( let == '\'' ) { pos[0] = '\\'; pos[1] = '\''; pos = pos + 2; }
        else if( let == '"'  ) { pos[0] = '\\'; pos[1] = '"';  pos = pos + 2; }
        else {
            pos[0] = let;
            pos++;
        }
        
        i++;
    }
    buffer->pos = pos;
}

char hex2char( char b1, char b2 ) {
    char ret = 0x00;
    if( b1 >= '0' && b1 <= '9' ) {
        ret += b1 - '0';
    }
    else if( b1 >= 'A' && b1 <= 'F' ) {
        ret += b1 - 'A' + 10;
    }
    else {
        // invalid
    }
    ret *= 16;
    if( b2 >= '0' && b2 <= '9' ) {
        ret += b2 - '0';
    }
    else if( b2 >= 'A' && b2 <= 'F' ) {
        ret += b2 - 'A' + 10;
    }
    else {
        // invalid
    }
    return ret;
}