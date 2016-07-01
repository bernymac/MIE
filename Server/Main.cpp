//
//  Main.cpp
//  MIE
//
//  Created by Bernardo Ferreira on 30/04/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#include "Server.h"
#include "MIEServer.h"
#include "MIEServerMT.h"
#include "SSEServer.h"
#include "CashServer.hpp"
#include "PaillierCashServer.hpp"

int main(int argc, const char * argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);
    Server* server;
    if (argc != 2) {
        printf("Incorrect number of arguments. Please give a server name, e.g. \"mie\", \"mieMT\", \"sse\", \"Cash\" and \"PaillierCash\"\n");
        return 0;
    } else if (strcasecmp(argv[1], "mie") == 0)
        server = new MIEServer();
    else if (strcasecmp(argv[1], "mieMT") == 0)
        server = new MIEServerMT();
    else if (strcasecmp(argv[1], "sse") == 0)
        server = new SSEServer();
    else if (strcasecmp(argv[1], "cash") == 0)
        server = new CashServer();
    else if (strcasecmp(argv[1], "PaillierCash") == 0)
        server = new PaillierCashServer();
    else {
        printf("Server command not recognized! Available Servers: \"mie\", \"mieMT\", \"sse\", \"Cash\" and \"PaillierCash\"\n");
        return 0;
    }
    server->startServer();
}

