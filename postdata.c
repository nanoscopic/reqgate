// Copyright (C) 2018 David Helkowski

#include "postdata.h"
#include<stdio.h>
#include<string.h>

postdatac::postdatac() {
    memset( &callbacks, 0, sizeof( multipart_parser_settings ) );
    callbacks.on_header_field = (multipart_data_cb) &partname_callback;
    callbacks.on_header_value = (multipart_data_cb) &partvalue_callback;
    callbacks.on_part_data = (multipart_data_cb) &data_callback;
    firstVal = lastVal = 0;
    type = 0;
}

void postdatac::parseUrlencoded( char *data, int len ) {
    int state = 0;
    int nameStart = 0;
    int valStart = 0;
    int i;
    for( i=0;i<len;i++ ) {
        char let = data[i];
        if( state == 0 ) { // name
            if( let != '=' ) continue;
            onname( &data[nameStart], i - nameStart );
            valStart = i + 1;
            state = 1;
            continue;
        }
        if( state == 1 ) { // value
            if( let != '&' ) continue;
            ondata( &data[valStart], i - valStart );
            nameStart = i + 1;
            state = 0;
            continue;
        }
    }
    data_callback( ( void *) this, &data[valStart], i - valStart );
    this->type = POST_URLENC;
}

void postdatac::rawPost( char *data, int len ) {
    this->rawPostData = data;
    this->rawPostLen = len;
    this->type = POST_RAW;
}

void postdatac::parseMultipart( char *boundary, char *data, int len ) {
    this->parser = multipart_parser__new( boundary, &callbacks );
    this->parser->data = this;
    multipart_parser_execute( this->parser, data, len );
    free( this->parser );
    this->type = POST_MULTIPART;
}

int postdatac::partname_callback( void *p, const char *name, size_t len ) {
    postdatac* self = ( postdatac * ) p;
    //1234567890123456789
    //Content-Disposition
    if( len == 19 && !strncmp( name, "Content-Disposition", 19 ) ) self->isContent = 1;
    else self->isContent = 0;
    //printf("Name:%.*s\n", len, name );
    return 0;
}

void postdatac::onname( char *value, size_t len ) {
    printf("Name:%.*s\n", (int) len, value );
    postvalType *newVal = new postvalType;
    memset( newVal, 0, sizeof( postvalType ) );
    newVal->name = value;
    newVal->namelen = len;
    if( !firstVal ) firstVal = lastVal = newVal;
    else {
        lastVal->next = newVal;
        lastVal = newVal;
    }
    firstData = 1;
}

int postdatac::partvalue_callback( void *p, const char *value, size_t len ) {
    postdatac* self = ( postdatac * ) p;
    if( self->isContent ) {
        //12345678901234567
        //form-data; name="
        if( len > 17 && !strncmp( value, "form-data; name=\"", 17 ) ) {
            self->onname( (char *) &value[ 17 ], len - 18 );
        }
    }
    //
    return 0;
}

void postdatac::ondata( const char *value, size_t len ) {
    
    if( !firstData ) { //&& ( lastVal->value + lastVal->valuelen ) == value ) {
        lastVal->valuelen += len;
        printf("Size increase: %i, Total:%i\n", (int) len, lastVal->valuelen );
    }
    else {
        printf("Data:%.*s\n", (int) len, value );
    }
    lastVal->value = ( char * ) value;
    lastVal->valuelen = len;
    firstData = 0;
}

int postdatac::data_callback( void *p, const char *value, size_t len ) {
    postdatac* self = ( postdatac * ) p;
    self->ondata( value, len );
    return 0;
}

void postdatac::toxml( str_buffer *buffer ) {
    if( this->type == POST_RAW ) {
        //json_to_xml( buffer, "json", this->rawPostData, this->rawPostLen );
    }
    else {
        postvalType *postval = this->firstVal;
        while( postval ) {
            //body_to_xml( buffer, postval->name, postval->namelen, postval->value, postval->valuelen );
            postval = postval->next;
        }
    }    
}
   