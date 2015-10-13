Server : Main.o MIEServer.o SSEServer.o ServerUtil.o CashServer.o
	g++ -L /usr/local/lib/ -L /usr/local/ssl/lib -lz -ldl -g -o Server Main.o MIEServer.o SSEServer.o ServerUtil.o CashServer.o

Main.o : Main.cpp
	g++ -I /usr/local/ssl/include -c Main.cpp
MIEServer.o : MIEServer.cpp
	g++ -I /usr/local/ssl/include -c MIEServer.cpp
SSEServer.o : SSEServer.cpp
	g++ -I /usr/local/ssl/include -c SSEServer.cpp
ServerUtil.o : ServerUtil.cpp
	g++ -I /usr/local/ssl/include -c ServerUtil.cpp
CashServer.o : CashServer.cpp
	g++ -I /usr/local/ssl/include -c CashServer.cpp
clean :
	rm Server Main.o MIEServer.o SSEServer.o ServerUtil.o CashServer.o
