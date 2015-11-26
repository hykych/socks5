#binary to be created
BINS := socks5

#libs to be created
LIBS := liblog.so libthread.so
#-----------------------------------------------

#compiler tool
CXX := g++

#compile option to executables
CXXFLAGS := --std=c++11 -g -w -O3 -Iinc

#compile option to .so
SOFLAGS := --std=c++11 -g -w -shared -fPIC -Iinc

LDFLAGS := -Wl,-rpath,bin,-rpath, \
    -Lbin \
         -lpthread -llog -lev -lthread
#vpath indicate the searching path of the according file type
SRCDIR := src $(shell ls -d src/*)
vpath %.cpp $(SRCDIR)
vpath %.h inc
vpath %.so bin
vpath % bin

.PHONY: all libs bins

all : lib bins

lib : $(LIBS)

bins : $(BINS)

clean :
	cd bin;\
        rm -f $(LIBS);\
	rm -f $(BINS);\
	cd ..;


#common rules goes here, if the compiling procedure of your module matches one,
# no need to list it in SpecialRules
%.so : %.cpp
	$(CXX) $^ $(SOFLAGS) -o $@
	mv $@ bin/
socks5 : socks5.cpp
	$(CXX) $^ $(CXXFLAGS) -o $@ $(LDFLAGS)
	mv $@ bin/
