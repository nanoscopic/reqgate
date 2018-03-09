// Copyright (C) 2018 David Helkowski

#ifndef __STATIC_H
#define __STATIC_H
#include "xmlbare/parser.h"
#include "urlpath.h"

class mapped_folderc {
    
    public:
        urlpathc *urlpath;
    char *local;
    mapped_folderc *next;
    mapped_folderc( char *url, char *local );
    ~mapped_folderc();
};

class staticsc {
    mapped_folderc *firstFolder;
    mapped_folderc *lastFolder;
    public:
    staticsc( nodec *conf );
    mapped_folderc *ismapped( urlpathc *file );
    ~staticsc();
};

#endif
