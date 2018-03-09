// Copyright (C) 2018 David Helkowski

#include "urlpath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void urlpathc::decode( char *in, size_t len ) {
    this->str = new char[ len + 1 ];//(char *) malloc( len + 1 );
    memcpy( this->str, in, len );
    
    char *readpos = this->str;
    char *writepos = this->str;
    char code[3] = "00";
    char dup = 0;
    while( --len ) {
        char let = *readpos;
        if( let == '%' && len >= 2 ) {
            code[0] = *(++readpos);
            code[1] = *(++readpos);
            let = (char) strtoul( code, NULL, 16 );
            *writepos = let;
            dup = 1;
        }
        else if( dup ) *writepos = let;
        if( let == '/' ) this->slashcnt++;
        readpos++;
        writepos++;
    }
    this->len = writepos - str + 1;
    this->str[ this->len ] = 0;
}

char *urlpathc::getfile() {
    int lastSlashI = this->slashcnt - 1;
    int lastSlashPos = this->slashpos[ lastSlashI ];
    if( lastSlashPos == ( this->len - 1 ) ) return 0; // ends in a slash...
    return &( this->str[ lastSlashPos + 1 ] );
}

urlpathc::urlpathc( char *path ) {
    this->slashcnt = 0;
    int l;
    for( l=0;l<500;l++ ) {
        char let = path[l];
        if( !let ) break;
        if( let == '/' ) this->slashcnt++;
    }
    if( this->slashcnt ) {
        this->slashpos = new int[ this->slashcnt ];
        int si = 0;
        for( int i=0;i<500;i++ ) {
            char let = path[i];
            if( !let ) break;
            if( let == '/' ) {
                this->slashpos[ si++ ] = i;
                if( si == this->slashcnt ) break;
            }
        }
    }
    this->len = l;
    this->str = new char[ l + 1 ];
    memcpy( this->str, path, l + 1 );
}

urlpathc::urlpathc( char *path, int len ) {
    if( len && path[0] == '/' ) {
        path++;
        len--;
    }
    this->slashcnt = 0;
    decode( path, len );
    if( this->slashcnt ) {
        this->slashpos = new int[ this->slashcnt ];
        int si = 0;
        for( int i=0;i<this->len;i++ ) {
            char let = this->str[i];
            if( let == '/' ) {
                this->slashpos[ si++ ] = i;
                if( si == this->slashcnt ) break;
            }
        }
    }
}

urlpathc::~urlpathc() {
    delete this->str;
    if( this->slashcnt ) delete this->slashpos;
}

char urlpathc::infolder( urlpathc *folder ) {
    //printf("Is %s in folder %s?\n", this->str, folder->str );
    if( folder->slashcnt != ( this->slashcnt - 1 ) ) return 0;
    int folderStart = 0;
    int folderEnd = 0;
    int thisStart = 0;
    int thisEnd = 0;
    if( folder->slashcnt ) {
        folderEnd = folder->slashpos[ 0 ];
    }
    else {
        folderEnd = folder->len;
    }
    thisEnd = this->slashpos[ 0 ];
    
    for( int part=0;part<=folder->slashcnt;part++ ) {
        //printf("folderStart %i thisStart %i folderEnd %i thisEnd %i\n", folderStart, thisStart, folderEnd, thisEnd );
        if( folderStart != thisStart || folderEnd != thisEnd ) return 0;
        int len = folderEnd - folderStart;
        char *folderStr = &folder->str[ folderStart ];
        char *thisStr = &this->str[ folderStart ];
        //printf("Comparing %.*s with %.*s\n", len, folderStr, len, thisStr );
        if( strncmp( folderStr, thisStr, len ) ) return 0;
    }
    return 1;
}