// Copyright (C) 2018 David Helkowski

#include "xmlbare/parser.h"
#include "static.h"
#include "urlpath.h"
staticsc::staticsc( nodec *conf ) {
    arrc *folders = conf->getnodes( (char *) "folder" );
    if( folders ) {
        for( int i=0;i<folders->count;i++ ) {
            nodec *folderNode = (nodec *) folders->items[ i ];
            char *url = folderNode->getattval( (char *) "url" );
            char *local = folderNode->getattval( (char *) "local" );
            if( !url || !local ) continue;
            mapped_folderc *folder = new mapped_folderc( url, local );
            if( !i ) this->firstFolder = this->lastFolder = folder;
            else {
                this->lastFolder->next = folder;
                this->lastFolder = folder;
            }
        }
    }
    else {
        this->firstFolder = 0;
        this->lastFolder = 0;
    }
}
staticsc::~staticsc() {
}

mapped_folderc *staticsc::ismapped( urlpathc *file ) {
    mapped_folderc *folder = this->firstFolder;
    while( folder ) {
        if( file->infolder( folder->urlpath ) ) return folder;
        folder = folder->next;
    }
    return 0;
}

mapped_folderc::mapped_folderc( char *url, char *local ) {
    this->urlpath = new urlpathc( url );
    this->local = local;
    this->next = 0;
}

mapped_folderc::~mapped_folderc() {
    delete this->urlpath;
}