#make run build accept arguments
ifeq (run,$(firstword $(MAKECMDGOALS)))
RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
$(eval $(RUN_ARGS):;@:)
endif

Client : Main.o CashClient.o PaillierCashClient.o CashCrypt.o EnglishAnalyzer.o MIEClient.o SBE.o SSEClient.o SSECrypt.o TextCrypt.o Util.o paillier.o
	g++ -o Client Main.o CashClient.o CashCrypt.o PaillierCashClient.o EnglishAnalyzer.o MIEClient.o SBE.o SSEClient.o SSECrypt.o TextCrypt.o Util.o paillier.o -L/usr/local/opencv/opencv2.4/lib -lopencv_core -lopencv_nonfree -lopencv_highgui -lopencv_features2d -lz -lcrypto -lgmp -ldl -lpthread

debug : Main.debug CashClient.debug PaillierCashClient.debug CashCrypt.debug EnglishAnalyzer.debug MIEClient.debug SBE.debug SSEClient.debug SSECrypt.debug TextCrypt.debug Util.debug paillier.debug
	g++ -o Client Main.o CashClient.o CashCrypt.o PaillierCashClient.o EnglishAnalyzer.o MIEClient.o SBE.o SSEClient.o SSECrypt.o TextCrypt.o Util.o paillier.o -L/usr/local/opencv/opencv2.4/lib -lopencv_core -lopencv_nonfree -lopencv_highgui -lopencv_features2d -lz -lcrypto -lgmp -ldl -lpthread

Main.o : Main.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c Main.cpp

Main.debug : Main.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c Main.cpp

CashClient.o : CashClient.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c CashClient.cpp

CashClient.debug : CashClient.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c CashClient.cpp

PaillierCashClient.o : PaillierCashClient.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c PaillierCashClient.cpp

PaillierCashClient.debug : PaillierCashClient.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c PaillierCashClient.cpp

CashCrypt.o : CashCrypt.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c CashCrypt.cpp

CashCrypt.debug : CashCrypt.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c CashCrypt.cpp

EnglishAnalyzer.o : EnglishAnalyzer.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c EnglishAnalyzer.cpp

EnglishAnalyzer.debug : EnglishAnalyzer.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c EnglishAnalyzer.cpp

MIEClient.o : MIEClient.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c MIEClient.cpp

MIEClient.debug : MIEClient.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c MIEClient.cpp

SBE.o : SBE.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c SBE.cpp

SBE.debug : SBE.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c SBE.cpp

SSEClient.o : SSEClient.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c SSEClient.cpp

SSEClient.debug : SSEClient.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c SSEClient.cpp

SSECrypt.o : SSECrypt.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c SSECrypt.cpp

SSECrypt.debug : SSECrypt.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c SSECrypt.cpp

TextCrypt.o : TextCrypt.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c TextCrypt.cpp

TextCrypt.debug : TextCrypt.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c TextCrypt.cpp

Util.o : Util.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c Util.cpp

Util.debug : Util.cpp
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c Util.cpp

paillier.o : paillier.c
	gcc -c paillier.c

paillier.debug : paillier.c
	gcc -c -g -O0 paillier.c

clean :
	rm Client Main.o CashClient.o CashCrypt.o PaillierCashClient.o EnglishAnalyzer.o MIEClient.o SBE.o SSEClient.o SSECrypt.o TextCrypt.o Util.o paillier.o core

run : Client
	LD_LIBRARY_PATH=/usr/local/opencv/opencv2.4/lib/:$$LD_LIBRARY_PATH ./Client $(RUN_ARGS)

.PHONY: run clean
