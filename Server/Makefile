Server : Main.o MIEServer.o SSEServer.o ServerUtil.o CashServer.o
	g++ -o Server Main.o MIEServer.o SSEServer.o ServerUtil.o CashServer.o `pkg-config --libs zlib` `pkg-config --libs opencv` `pkg-config --libs openssl` -ldl

Main.o : Main.cpp
	g++ -std=c++14 -I /usr/local/ssl/include -c Main.cpp
MIEServer.o : MIEServer.cpp
	g++ -std=c++14 -I /usr/local/ssl/include -c MIEServer.cpp
SSEServer.o : SSEServer.cpp
	g++ -std=c++14 -I /usr/local/ssl/include -c SSEServer.cpp
ServerUtil.o : ServerUtil.cpp
	g++ -std=c++14 -I /usr/local/ssl/include -c ServerUtil.cpp
CashServer.o : CashServer.cpp
	g++ -std=c++14 -I /usr/local/ssl/include -c CashServer.cpp
clean :
	rm Server Main.o MIEServer.o SSEServer.o ServerUtil.o CashServer.o
