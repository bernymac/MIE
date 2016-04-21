#make run build accept arguments
ifeq (run,$(firstword $(MAKECMDGOALS)))
RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
$(eval $(RUN_ARGS):;@:)
endif

Server : Main.o MIEServer.o MIEServerMT.o SSEServer.o ServerUtil.o CashServer.o PaillierCashServer.o paillier.o
	g++ -o Server Main.o MIEServer.o MIEServerMT.o SSEServer.o ServerUtil.o CashServer.o PaillierCashServer.o paillier.o -L/usr/local/opencv/opencv3.1/lib -lopencv_core -lopencv_features2d -lcrypto -lz -lgmp -ldl

debug : Main.debug MIEServer.debug MIEServerMT.debug SSEServer.debug ServerUtil.debug CashServer.debug PaillierCashServer.debug paillier.debug
	g++ -o Server Main.o MIEServer.o MIEServerMT.o SSEServer.o ServerUtil.o CashServer.o PaillierCashServer.o paillier.o -L/usr/local/opencv/opencv3.1/lib -lopencv_core -lopencv_features2d -lcrypto -lz -lgmp -ldl

Main.o : Main.cpp
	g++ -std=c++11 -I /usr/local/opencv/opencv3.1/include -c Main.cpp

MIEServer.o : MIEServer.cpp
	g++ -std=c++11 -I /usr/local/opencv/opencv3.1/include -c MIEServer.cpp

MIEServerMT.o : MIEServerMT.cpp
	g++ -std=c++11 -I /usr/local/opencv/opencv3.1/include -c MIEServerMT.cpp

SSEServer.o : SSEServer.cpp
	g++ -std=c++11 -I /usr/local/opencv/opencv3.1/include -c SSEServer.cpp

ServerUtil.o : ServerUtil.cpp
	g++ -std=c++11 -I /usr/local/opencv/opencv3.1/include -c ServerUtil.cpp

CashServer.o : CashServer.cpp
	g++ -std=c++11 -I /usr/local/opencv/opencv3.1/include -c CashServer.cpp

PaillierCashServer.o : PaillierCashServer.cpp
	g++ -std=c++11 -I /usr/local/opencv/opencv3.1/include -c PaillierCashServer.cpp

paillier.o : paillier.c
	gcc -c paillier.c

Main.debug : Main.cpp
	g++ -std=c++11 -I /usr/local/opencv/opencv3.1/include -g -O0 -c Main.cpp

MIEServer.debug : MIEServer.cpp
	g++ -std=c++11 -I /usr/local/opencv/opencv3.1/include -g -O0 -c MIEServer.cpp

MIEServerMT.debug : MIEServerMT.cpp
	g++ -std=c++11 -I /usr/local/opencv/opencv3.1/include -g -O0 -c MIEServerMT.cpp

SSEServer.debug : SSEServer.cpp
	g++ -std=c++11 -I /usr/local/opencv/opencv3.1/include -g -O0 -c SSEServer.cpp

ServerUtil.debug : ServerUtil.cpp
	g++ -std=c++11 -I /usr/local/opencv/opencv3.1/include -g -O0 -c ServerUtil.cpp

CashServer.debug : CashServer.cpp
	g++ -std=c++11 -I /usr/local/opencv/opencv3.1/include -g -O0 -c CashServer.cpp

PaillierCashServer.debug : PaillierCashServer.cpp
	g++ -std=c++11 -I /usr/local/opencv/opencv3.1/include -g -O0 -c PaillierCashServer.cpp

paillier.debug : paillier.c
	gcc -g -O0 -c paillier.c

clean :
	rm Server Main.o MIEServer.o MIEServerMT.o SSEServer.o ServerUtil.o CashServer.o PaillierCashServer.o paillier.o core

run : Server
	LD_LIBRARY_PATH=/usr/local/opencv/opencv3.1/lib/:$$LD_LIBRARY_PATH ./Server $(RUN_ARGS)

.PHONY: run clean
