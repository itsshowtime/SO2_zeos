#!/usr/bin/env bash

apt-get update

# Needed tools to compile bochs...
apt-get -y install build-essential
apt-get -y install libx11-dev
apt-get -y install libgtk2.0-dev

# Needed tools for Zeos
apt-get -y install vim
apt-get -y install bin86
apt-get -y install gdb
apt-get -y install git

# Compile BOCHS
mkdir tmp
cd tmp
BOCHS=bochs-2.6.7
wget http://sourceforge.net/projects/bochs/files/bochs/2.6.7/bochs-2.6.7.tar.gz/download -O ${BOCHS}.tar.gz

tar zxf ${BOCHS}.tar.gz

# ... without GDB
mkdir build
cd build
LDFLAGS=-pthread ../${BOCHS}/configure --enable-debugger --enable-disasm --enable-x86-debugger --enable-readline --with-x --prefix=/opt/bochs
make
make install
cd ..

# ... with GDB
mkdir build-gdb
cd build-gdb
../${BOCHS}/configure --enable-gdb-stub --with-x --prefix=/opt/bochs_gdb
make
make install
cd ..

# Create links
ln -sf /opt/bochs/bin/bochs /usr/local/bin/bochs_nogdb
ln -sf /opt/bochs_gdb/bin/bochs /usr/local/bin/bochs

cd ..

# Clean environment
rm -rf tmp


