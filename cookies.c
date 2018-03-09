// Copyright (C) 2018 David Helkowski

#include"cookies.h"
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>

cookiesc::cookiesc() {
    cookieCnt = 0;
    firstCookie = lastCookie = 0;
}
cookiesc::~cookiesc() {
    if( cookieCnt ) {
        if( cookieCnt == 1 ) {
            delete firstCookie;
        }
        else {
            cookieType *cookie = firstCookie;
            while( cookie ) {
                cookieType *nextCookie = cookie->next;
                if( cookie->expires ) free( cookie->expires );
                delete cookie;
                cookie = nextCookie;
            }
        }
    }
}
void cookiesc::output( str_buffer *buffer ) {
    cookieType *cookie = firstCookie;
    while( cookie ) {
        str_appendZ( buffer, "Set-Cookie: " );
        str_append( buffer, cookie->name, cookie->namelen );
        str_appendZ( buffer, "=" );
        str_append( buffer, cookie->value, cookie->valuelen );
        
        if( cookie->path ) {
            str_appendZ( buffer, "; Path=" );
            str_append( buffer, cookie->path, cookie->pathlen );
        }
        else {
            str_appendZ( buffer, "; Path=/" );
        }
        
        if( cookie->expires ) {
            str_appendZ( buffer, "; Expires=" );
            str_append( buffer, cookie->expires, cookie->expireslen );
        }
        
        str_appendZ( buffer, "\r\n" );
            
        cookie = cookie->next;
    }
}
void cookiesc::parseCookie( char *data, int datalen ) {
    cookieType *cookie = new cookieType;
    memset( cookie, 0, sizeof( cookieType ) );
}

void cookiesc::cookieExpireSeconds( cookieType *cookie, int offset ) {
    time_t curtime;
    time( &curtime );
    
    struct tm curtime_tm = *localtime( &curtime );
    curtime_tm.tm_sec += offset;
    /*time_t offsetTime = */mktime( &curtime_tm ); // normalize tm
    
    char rfc822[200];
    strftime( rfc822, 200, "%a, %d %b %y %T %z", &curtime_tm );
    printf("Expiration %s\n", rfc822 );
    
    cookie->expires = strdup( rfc822 );
    cookie->expireslen = strlen( rfc822 );
}

cookieType *cookiesc::addCookie( char *nameZ, char *valueZ, char *pathZ, char *expiresZ ) {
    if( !nameZ || !valueZ ) return 0;
    
    cookieType *cookie = new cookieType;
    memset( cookie, 0, sizeof( cookieType ) );
    cookie->name = nameZ;
    cookie->namelen = strlen( nameZ );
    cookie->value = valueZ;
    cookie->valuelen = strlen( valueZ );
    if( pathZ ) {
        cookie->path = pathZ;
        cookie->pathlen = strlen( pathZ );
    }
    if( expiresZ ) {
        cookie->expires = strdup( expiresZ );
        cookie->expireslen = strlen( expiresZ );
    }
    
    if( !lastCookie ) firstCookie = lastCookie = cookie;
    else {
        lastCookie->next = cookie;
        lastCookie = cookie;
    }
    return cookie;
}