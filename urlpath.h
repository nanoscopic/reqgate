// Copyright (C) 2018 David Helkowski

#ifndef __URLPATH_H
#define __URLPATH_H
#include<stdio.h>
#include<string.h>
class urlpathc {
    int len;
    int slashcnt;
    int *slashpos;
    void decode( char *in, size_t len );
    public:
    char *str;
    urlpathc( char *path );
    urlpathc( char *path, int len ); // specific initializer from raw request data
    char infolder( urlpathc *folder );
    char *getfile();
    ~urlpathc();
};

#endif