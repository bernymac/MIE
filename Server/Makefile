Server : Main.o MIEServer.o SSEServer.o ServerUtil.o CashServer.o
	g++ -g -lstdc++ -L /usr/local/ssl/lib/libcrypto.a -L /usr/local/ssl/lib/libssl.a -L /usr/local/lib/libz.a -I 'pkg-config --libs opencv' -o Server Main.o MIEServer.o SSEServer.o ServerUtil.o CashServer.o

Main.o : Main.cpp
	gcc -I /usr/local/ssl/include -c Main.cpp
MIEServer.o : MIEServer.cpp
	gcc -I /usr/local/ssl/include -c MIEServer.cpp
SSEServer.o : SSEServer.cpp
	gcc -I /usr/local/ssl/include -c SSEServer.cpp
ServerUtil.o : ServerUtil.cpp
	gcc -I /usr/local/ssl/include -c ServerUtil.cpp
CashServer.o : CashServer.cpp
	gcc -I /usr/local/ssl/include -c CashServer.cpp
clean :
	rm Server Main.o MIEServer.o SSEServer.o ServerUtil.o CashServer.o
