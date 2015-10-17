//
//  Main.cpp
//  MIE
//
//  Created by Bernardo Ferreira on 30/04/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#include "MIEServer.h"
#include "MIEServerMT.h"
#include "SSEServer.h"
#include "CashServer.hpp"

int main(int argc, const char * argv[]) {
    if (argc != 2)
        printf("Incorrect number of arguments. Please give a server name, e.g. \"MIE\", \"SSE\" or \"Cash\"\n");
    else if (strcasecmp(argv[1], "-mie") == 0)
        MIEServer server;
    else if (strcasecmp(argv[1], "-mieMT") == 0)
        MIEServerMT server;
    else if (strcasecmp(argv[1], "-sse") == 0)
        SSEServer server;
    else if (strcasecmp(argv[1], "-cash") == 0)
        CashServer server;
    else
        printf("Server command not recognized! Available Servers: \"mie\", \"mieMT\", \"sse\" and \"Cash\"\n");
}

