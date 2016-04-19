Client : Main.o CashClient.o PaillierCashClient.o CashCrypt.o \
		EnglishAnalyzer.o MIEClient.o SBE.o SSEClient.o SSECrypt.o \
		TextCrypt.o Util.o
	g++ -o Client Main.o CashClient.o CashCrypt.o PaillierCashClient.o EnglishAnalyzer.o \
		MIEClient.o SBE.o SSEClient.o SSECrypt.o TextCrypt.o Util.o \
		-L/usr/local/opencv/opencv2.4/lib -lopencv_core -lopencv_features2d \
		-lopencv_highgui -lopencv_imgcodecs -lopencv_nonfree -lz -lcrypto -lgmp -ldl

Main.o : Main.cpp MIEClient.o SSEClient.o CashClient.o
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c Main.cpp
CashClient.o : CashClient.cpp EnglishAnalyzer.o CashCrypt.o Util.o
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c CashClient.cpp
PaillierCashClient.o : PaillierCashClient.cpp CashClient.o
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c PaillierCashClient.cpp
CashCrypt.o : CashCrypt.cpp CashCrypt.hpp Util.h
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c CashCrypt.cpp
EnglishAnalyzer.o : EnglishAnalyzer.cpp EnglishAnalyzer.h PorterStemmer.c Util.h
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c EnglishAnalyzer.cpp
MIEClient.o : MIEClient.cpp MIEClient.h EnglishAnalyzer.h SBE.h TextCrypt.h Util.h
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c MIEClient.cpp
SBE.o : SBE.cpp SBE.h Util.h
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c SBE.cpp
SSEClient.o : SSEClient.cpp SSEClient.h EnglishAnalyzer.h SSECrypt.h TextCrypt.h Util.h
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c SSEClient.cpp
SSECrypt.o : SSECrypt.cpp SSECrypt.h Util.h
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c SSECrypt.cpp
TextCrypt.o : TextCrypt.cpp TextCrypt.h Util.h
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c TextCrypt.cpp
Util.o : Util.cpp Util.h portable_endian.h
	g++ -std=gnu++98 -I /usr/local/opencv/opencv2.4/include -c Util.cpp
clean :
	rm Client Main.o CashClient.o CashCrypt.o PaillierCashClient.o EnglishAnalyzer.o \
		MIEClient.o SBE.o SSEClient.o SSECrypt.o TextCrypt.o Util.o
