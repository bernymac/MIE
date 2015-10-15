//
//  Main.cpp
//  MIE
//
//  Created by Bernardo Ferreira on 30/04/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#include "MIEServer.h"
#include "SSEServer.h"
#include "CashServer.hpp"

int main(int argc, const char * argv[]) {
    if (argc != 2)
        printf("Incorrect number of arguments. Please give a server name, e.g. \"MIE\", \"SSE\" or \"Cash\"\n");
    else
        if (strcasecmp(argv[1], "MIE") == 0)
            MIEServer server;
        else
            if (strcasecmp(argv[1], "SSE") == 0)
                SSEServer server;
            else
                if (strcasecmp(argv[1], "Cash") == 0)
                    CashServer server;
                else
                    printf("Server command not recognized! Available Servers: \"MIE\", \"SSE\" and \"Cash\"\n");
}

