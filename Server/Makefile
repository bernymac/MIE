#make run build accept arguments
ifeq (run,$(firstword $(MAKECMDGOALS)))
RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
$(eval $(RUN_ARGS):;@:)
endif

Server : Main.o MIEServer.o MIEServerMT.o SSEServer.o ServerUtil.o CashServer.o PaillierCashServer.o paillier.o
	g++ -o Server Main.o MIEServer.o MIEServerMT.o SSEServer.o ServerUtil.o CashServer.o PaillierCashServer.o paillier.o \
		-L/usr/local/opencv/opencv3.1/lib -lopencv_core -lopencv_features2d -lcrypto -lz -lgmp -ldl

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

.PHONY: clean
clean :
	rm Server Main.o MIEServer.o MIEServerMT.o SSEServer.o ServerUtil.o CashServer.o PaillierCashServer.o paillier.o

.PHONY: config
config :
	export LD_LIBRARY_PATH=/usr/local/opencv/opencv3.1/lib/:$$LD_LIBRARY_PATH

.PHONY: run
run : Server
	./Server $(RUN_ARGS)
