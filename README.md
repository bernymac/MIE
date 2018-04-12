MIE is a searchable encryption framework for multimodal data (i.e. data containing multiple media formats simultaneously) especially designed for supporting mobile devices and optimizing their storage, computation, and battery resources. It allows multiple users to securely store, update, and search multimodal data in commercial clouds.

If you are to use this project, please cite: Ferreira, B., Leitao, J., & Domingos, H. (2017, June). Multimodal Indexable Encryption for Mobile Cloud-based Applications. In Dependable Systems and Networks (DSN), 2017 IEEE/IFIP 47th International Conference on. IEEE.


Requirements:
OpenCV 3.0.X (server)
OpenCV 2.4.X (client)


Build Instructions (assuming a new Ubuntu machine):


- First Steps:

sudo apt-get update
sudo apt-get upgrade
sudo apt-get install unzip
sudo apt-get install build-essential
sudo apt-get install cmake git libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev
sudo apt-get install python-dev python-numpy libtbb2 libtbb-dev libjpeg-dev libpng-dev libtiff-dev libjasper-dev libdc1394-22-dev
sudo apt-get install m4


- Building OpenCV for the client:

TODO


- Building OpenCV for the server:

wget https://github.com/opencv/opencv/archive/3.1.0.zip
unzip 3.1.0.zip
cd opencv-3.1.0/
mkdir build
cd build
cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=/usr/local/opencv/opencv3.1 ..
make -j7
sudo make install


- Building OpenSSL:

wget https://www.openssl.org/source/openssl-1.0.2o.tar.gz
tar -xzvf openssl-1.0.2o.tar.gz
cd openssl-1.0.2o/
./config --prefix=/usr/local --openssldir=/usr/local/openssl
make
make test
sudo make install


- Building GMP:

wget https://gmplib.org/download/gmp/gmp-6.1.2.tar.xz
tar -zxfv gmp-6.1.2.tar.xz
cd gmp-6.1.2
./configure
make
make check
sudo make install


- Building and running the Server:

git clone https://github.com/bernymac/MIE.git
cd MIE/Server/
make
make run X


- Building and running the Client:

(if not done already for the server) git clone https://github.com/bernymac/MIE.git
cd MIE/MIE/
make
make run X
