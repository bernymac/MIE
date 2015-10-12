Server : Main.o MIEServer.o SSEServer.o ServerUtil.o CashServer.o
	gcc -o Server Main.o MIEServer.o SSEServer.o ServerUtil.o CashServer.o

Main.o : Main.cpp
	gcc -I /usr/local/ssl/include -L /usr/local/ssl/lib -c Main.cpp
MIEServer.o : MIEServer.cpp
	gcc -I /usr/local/ssl/include -L /usr/local/ssl/lib -c MIEServer.cpp
SSEServer.o : SSEServer.cpp
	gcc -I /usr/local/ssl/include -L /usr/local/ssl/lib -c SSEServer.cpp
ServerUtil.o : ServerUtil.cpp
	gcc -I /usr/local/ssl/include -L /usr/local/ssl/lib -c ServerUtil.cpp
CashServer.o : CashServer.cpp
	gcc -I /usr/local/ssl/include -L /usr/local/ssl/lib -c CashServer.cpp
clean :
	rm Server Main.o MIEServer.o SSEServer.o ServerUtil.o CashServer.o
