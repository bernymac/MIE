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
        printf("Incorrect number of arguments. Please pass only a server name, e.g. \"MIE\", \"SSE\" or \"Cash\"\n");
    else
        if (strcasecmp(argv[1], "MIE"))
            MIEServer server;
        else
            if (strcasecmp(argv[1], "SSE"))
                SSEServer server;
            else
                if (strcasecmp(argv[1], "Cash"))
                    CashServer server;
}

