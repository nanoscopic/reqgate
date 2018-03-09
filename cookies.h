// Copyright (C) 2018 David Helkowski

#ifndef __COOKIES_H
#define __COOKIES_H

#include"str_buffer.h"
struct cookie_s {
    char *name;
    int namelen;
    char *value;
    int valuelen;
    char *path;
    int pathlen;
    char *expires;
    int expireslen;
    struct cookie_s *next;
};
typedef struct cookie_s cookieType;

class cookiesc {
    cookieType *firstCookie;
    cookieType *lastCookie;
    int cookieCnt;
    public:
    cookiesc();
    ~cookiesc();
    void parseCookie( char *data, int datalen );
    cookieType *addCookie( char *nameZ, char *valueZ, char *pathZ, char *expiresZ );
    void cookieExpireSeconds( cookieType *cookie, int offset );
    void output( str_buffer *buffer );
};

#endif