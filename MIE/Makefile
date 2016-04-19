#make run build accept arguments
ifeq (run,$(firstword $(MAKECMDGOALS)))
RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
$(eval $(RUN_ARGS):;@:)
endif

Client : Main.o CashClient.o PaillierCashClient.o CashCrypt.o EnglishAnalyzer.o MIEClient.o SBE.o SSEClient.o SSECrypt.o \
		TextCrypt.o Util.o paillier.o
	g++ -o Client Main.o CashClient.o CashCrypt.o PaillierCashClient.o EnglishAnalyzer.o MIEClient.o SBE.o SSEClient.o \
		SSECrypt.o TextCrypt.o Util.o paillier.o -L/usr/local/opencv/opencv2.4/lib -lopencv_core -lopencv_features2d \
		-lopencv_highgui -lopencv_nonfree -lz -lcrypto -lgmp -ldl -lpthread

Main.o : Main.cpp MIEClient.o SSEClient.o CashClient.o
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c Main.cpp

CashClient.o : CashClient.cpp CashClient.hpp EnglishAnalyzer.o CashCrypt.o Util.o
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c CashClient.cpp

PaillierCashClient.o : PaillierCashClient.cpp PaillierCashClient.hpp CashClient.o paillier.o
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c PaillierCashClient.cpp

CashCrypt.o : CashCrypt.cpp CashCrypt.hpp Util.h
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c CashCrypt.cpp

EnglishAnalyzer.o : EnglishAnalyzer.cpp EnglishAnalyzer.h PorterStemmer.c Util.h
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c EnglishAnalyzer.cpp

MIEClient.o : MIEClient.cpp MIEClient.h EnglishAnalyzer.h SBE.h TextCrypt.h Util.h
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c MIEClient.cpp

SBE.o : SBE.cpp SBE.h Util.h
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c SBE.cpp

SSEClient.o : SSEClient.cpp SSEClient.h EnglishAnalyzer.h SSECrypt.h TextCrypt.h Util.h
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c SSEClient.cpp

SSECrypt.o : SSECrypt.cpp SSECrypt.h Util.h
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c SSECrypt.cpp

TextCrypt.o : TextCrypt.cpp TextCrypt.h Util.h
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c TextCrypt.cpp

Util.o : Util.cpp Util.h portable_endian.h
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -g -O0 -c Util.cpp

paillier.o : paillier.c paillier.h
	gcc -c -g -O0 paillier.c

clean :
	rm Client Main.o CashClient.o CashCrypt.o PaillierCashClient.o EnglishAnalyzer.o MIEClient.o SBE.o SSEClient.o SSECrypt.o \
		TextCrypt.o Util.o paillier.o

#export LD_LIBRARY_PATH=/usr/local/opencv/opencv2.4/lib/:$LD_LIBRARY_PATH
run : Client
	./Client $(RUN_ARGS)

.PHONY: run clean
