Server : Main.o MIEServer.o MIEServerMT.o SSEServer.o ServerUtil.o CashServer.o PaillierCashServer.o
	g++ -o Server Main.o MIEServer.o MIEServerMT.o SSEServer.o ServerUtil.o CashServer.o PaillierCashServer.o `pkg-config --libs zlib` `pkg-config --libs opencv` `pkg-config --libs openssl` `pkg-config --libs libgmp` -ldl

Main.o : Main.cpp
	g++ -std=c++11 -I /usr/local/ssl/include -c Main.cpp
MIEServer.o : MIEServer.cpp
	g++ -std=c++11 -I /usr/local/ssl/include -c MIEServer.cpp
MIEServerMT.o : MIEServerMT.cpp
	g++ -std=c++11 -I /usr/local/ssl/include -c MIEServerMT.cpp
SSEServer.o : SSEServer.cpp
	g++ -std=c++11 -I /usr/local/ssl/include -c SSEServer.cpp
ServerUtil.o : ServerUtil.cpp
	g++ -std=c++11 -I /usr/local/ssl/include -c ServerUtil.cpp
CashServer.o : CashServer.cpp
	g++ -std=c++11 -I /usr/local/ssl/include -c CashServer.cpp
paillier.o : paillier.c
	gcc -I /usr/local/ssl/include -c paillier.c
PaillierCashServer.o : PaillierCashServer.cpp paillier.o
	g++ -std=c++11 -I /usr/local/ssl/include -c PaillierCashServer.cpp
clean :
	rm Server Main.o MIEServer.o MIEServerMT.o SSEServer.o ServerUtil.o CashServer.o PaillierCashServer.o
