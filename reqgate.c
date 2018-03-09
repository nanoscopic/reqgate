// Copyright (C) 2018 David Helkowski

#include "server.h"

int main( int argc, char** argv ) {
    serverc server;
    
    server.readConf( (char *) "conf.xml" );
    server.init();
    server.dolisten();
    
    server.forkAccepts();
    
    return 0;
}