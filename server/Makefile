#binary to be created
BINS := socks5

#libs to be created
LIBS := liblog.so libthread.so libconfig.so
#-----------------------------------------------

#compiler tool
CXX := g++

#compile option to executables
CXXFLAGS := --std=c++11 -g -Wall -O3 -Iinc

#compile option to .so
SOFLAGS := --std=c++11 -g -shared -fPIC -Iinc

#vpath indicate the searching path of the according file type
SRCDIR := src $(shell ls -d src/*)
vpath %.c $(SRCDIR)
vpath %.h inc
vpath %.so bin
vpath % bin
vpath %.cpp $(SRCDIR)

.PHONY: all libs bins

all : lib bins

lib : $(LIBS)

bins : $(BINS)






