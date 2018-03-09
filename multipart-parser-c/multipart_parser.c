/*
Based on:
  https://github.com/iafonov/multipart-parser-c - commit 772639c 
  Copyright (C) 2012 Igor Afonov
  MIT License - http://www.opensource.org/licenses/mit-license.php
Sublicense/Modifications: ( the contents of this file )
  Copyright (C) 2017 David Helkowski
  CS-REF License - http://www.carbonstate.com/csref.htm
*/
#include "multipart_parser.h"
#include <stdio.h>
#include <string.h>

#define EMIT_DATA_CB(FOR, ptr, len) if( p->settings->on_##FOR && p->settings->on_##FOR(p->data, ptr, len) != 0 ) return i;

enum state {
    s_uninitialized = 1, s_start, s_start_boundary, s_header_field_start, s_header_field,
    s_headers_almost_done, s_header_value_start, s_header_value, s_header_value_almost_done,
    s_part_data_start, s_part_data, s_part_data_almost_boundary, s_part_data_boundary,
    s_part_data_almost_end, s_part_data_end, s_part_data_final_hyphen, s_end
};

multipart_parser* multipart_parser__new( const char *boundary, const multipart_parser_settings* settings ) {
    multipart_parser* p = ( multipart_parser* ) malloc( sizeof(multipart_parser) + strlen(boundary) + strlen(boundary) + 9 );
    
    strcpy( p->multipart_boundary, boundary );
    p->boundary_length = strlen( boundary );
    p->index = 0;
    p->state = s_start;
    p->settings = settings;
    
    return p;
}

size_t multipart_parser_execute( multipart_parser* p, const char *buf, size_t len ) {
    size_t i = 0;
    size_t mark = 0;
    
    while( i < len ) {
        char c = buf[i];
        switch( p->state ) {
            case s_start:
                p->index = 0;
                p->state = s_start_boundary;
                /* fallthrough */
            case s_start_boundary:
                if( p->index == p->boundary_length ) {
                    if( c != 0x0d ) return i;
                    p->index++;
                    break;
                } 
                else if( p->index == ( p->boundary_length + 1 ) ) {
                    if( c != 0x0a ) return i;
                    p->index = 0;
                    p->state = s_header_field_start;
                    break;
                }
                if( c != p->multipart_boundary[p->index] ) return i;
                p->index++;
                break;
            
            case s_header_field_start:
                mark = i;
                p->state = s_header_field;
                /* fallthrough */
            case s_header_field:
                if( c == 0x0d ) {
                    p->state = s_headers_almost_done;
                    break;
                }
                
                if( c == ':' ) {
                    EMIT_DATA_CB( header_field, buf + mark, i - mark );
                    p->state = s_header_value_start;
                    break;
                }
                
                if( ! ( c == '-' || ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ) ) return i;
                if( i == (len - 1) ) EMIT_DATA_CB(header_field, buf + mark, (i - mark) + 1);
                break;
            
            case s_headers_almost_done:
                if( c != 0x0a ) return i;
                p->state = s_part_data_start;
                break;
            
            case s_header_value_start:
                if( c == ' ' ) break;
                mark = i;
                p->state = s_header_value;
                /* fallthrough */
            case s_header_value:
                if( c == 0x0d ) {
                    EMIT_DATA_CB( header_value, buf + mark, i - mark );
                    p->state = s_header_value_almost_done;
                    break;
                }
                if( i == (len - 1) ) EMIT_DATA_CB( header_value, buf + mark, (i - mark) + 1 );
                break;
            
            case s_header_value_almost_done:
                if( c != 0x0a ) return i;
                p->state = s_header_field_start;
                break;
            
            case s_part_data_start:
                mark = i;
                p->state = s_part_data;
                /* fallthrough */
            case s_part_data:
                p->state = s_part_data_almost_boundary;
                if( i == (len - 1) ) {
                    EMIT_DATA_CB( part_data, buf + mark, (i - mark) + 1 );
                    p->state = s_part_data_start;
                }
                break;
            
            case s_part_data_almost_boundary:
                if( c == 0x0a ) {
                    p->state = s_part_data_boundary;
                    p->index = 0;
                    break;
                }
                i--;
                p->state = s_part_data;
                break;
            
            case s_part_data_boundary:
                if( p->multipart_boundary[ p->index ] != c ) {
                    i--;
                    p->state = s_part_data;
                    break;
                }
                if( ( ++ p->index ) == p->boundary_length ) {
                    p->state = s_part_data_almost_end;
                }
                break;
            
            case s_part_data_almost_end:
                if( c == '-' ) {
                    p->state = s_part_data_final_hyphen;
                    break;
                }
                if( c != 0x0d ) {
                    p->state = s_part_data;
                    break;
                }
                p->state = s_part_data_end;
                break;
            
            case s_part_data_final_hyphen:
                if( c != '-' ) return i;
                EMIT_DATA_CB( part_data, buf + mark, ( i - mark - p->boundary_length - 4 ) + 1 );
                p->state = s_end;
                break;
             
            case s_part_data_end:
                if( c != 0x0a ) return i;
                EMIT_DATA_CB( part_data, buf + mark, ( i - mark - p->boundary_length - 2 ) + 1 );
                p->state = s_header_field_start;
                break;
                           
            case s_end: break;
            default: return 0;
        }
        i++;
    }
    
    return len;
}
