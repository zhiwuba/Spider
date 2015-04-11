DIR_SRC=./
DIR_3RD=../3rd/
DIR_MYSQL_LIB=/usr/lib64/mysql/
TARGET=spider


CPPS=$(wildcard $(DIR_SRC)*.cpp)
CPPS+=$(wildcard $(DIR_SRC)/pugixml/*.cpp)
OBJS=$(patsubst %.cpp,%.o,$(CPPS))

CFLAGS=-g -O2 -DLINUX -D_DEBUG
CFLAGS+=-I $(DIR_3RD)jsoncpp/
CFLAGS+=-I $(DIR_3RD)mpir/include/
CFLAGS+=-I $(DIR_3RD)libevent/include/
CFLAGS+=-I $(DIR_3RD)crypto/include/
CFLAGS+=-I $(DIR_3RD)boost/

CC=g++
LIB=-L $(DIR_3RD)jsoncpp/ -ljsoncpp -pthread
LIB+=-L $(DIR_3RD)mpir/lib/ -lmpir
LIB+=-L $(DIR_3RD)libevent/lib/ -levent
LIB+=-L $(DIR_3RD)crypto/lib/ -lcryptopp
LIB+=-L $(DIR_3RD)boost/lib/ -lboost_filesystem -lboost_system
LIB+=-L $(DIR_MYSQL_LIB) -lmysqlclient
#LIB+=-L $(DIR_3RD)/hiredis -lhiredis

all:$(TARGET)
$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) -o $(TARGET) 

$(OBJS):%.o:%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(OBJS) $(TARGET)

install:
	cp $(TARGET) ../bin/


