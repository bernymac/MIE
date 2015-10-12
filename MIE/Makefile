Client : Main.o CashClient.o CashCrypt.o EnglishAnalyzer.o \
       MIEClient.o SBE.o SSEClient.o SSECrypt.o TextCrypt.o \
	   Util.o
	gcc -o Client Main.o CashClient.o CashCrypt.o EnglishAnalyzer.o \
      	 			MIEClient.o SBE.o SSEClient.o SSECrypt.o TextCrypt.o \
					Util.o

Main.o : Main.cpp MIEClient.o SSEClient.o CashClient.o
	gcc -I /usr/local/ssl/include -L /usr/local/ssl/lib -c Main.cpp
CashClient.o : CashClient.cpp CashClient.o EnglishAnalyzer.o CashCrypt.o Util.o
	gcc -I /usr/local/ssl/include -L /usr/local/ssl/lib -c CashClient.cpp
CashCrypt.o : CashCrypt.cpp CashCrypt.hpp Util.h
	gcc -I /usr/local/ssl/include -L /usr/local/ssl/lib -c CashCrypt.c
EnglishAnalyzer.o : EnglishAnalyzer.cpp EnglishAnalyzer.h PorterStemmer.c Util.h
	gcc -I /usr/local/ssl/include -L /usr/local/ssl/lib -c EnglishAnalyzer.cpp
MIEClient.o : MIEClient.cpp MIEClient.h EnglishAnalyzer.h SBE.h TextCrypt.h Util.h
	gcc -I /usr/local/ssl/include -L /usr/local/ssl/lib -c MIEClient.cpp
SBE.o : SBE.cpp SBE.h Util.h
	gcc -I /usr/local/ssl/include -L /usr/local/ssl/lib -c SBE.cpp
SSEClient.o : SSEClient.cpp SSEClient.h EnglishAnalyzer.h SSECrypt.h TextCrypt.h Util.h
	gcc -I /usr/local/ssl/include -L /usr/local/ssl/lib -c SSEClient.cpp
SSECrypt.o : SSECrypt.cpp SSECrypt.h Util.h
	gcc -I /usr/local/ssl/include -L /usr/local/ssl/lib -c SSECrypt.cpp
TextCrypt.o : TextCrypt.cpp TextCrypt.h Util.h
	gcc -I /usr/local/ssl/include -L /usr/local/ssl/lib -c TextCrypt.cpp
Util.o : Util.cpp Util.h portable_endian.h
	gcc -I /usr/local/ssl/include -L /usr/local/ssl/lib -c Util.cpp
clean :
	rm Client Main.o CashClient.o CashCrypt.o EnglishAnalyzer.o \
      	 			MIEClient.o SBE.o SSEClient.o SSECrypt.o TextCrypt.o \
					Util.o
