DOCDUMP_TARGET=ChampionList
COMMON_SOURCES=
DOCDUMP_SOURCES=ChampionList.cpp
CXX=g++

DEPENDENCIES = db_cxx indri z pthread m boost_filesystem boost_system
INCPATH=-I$(HOME)/include 
LDFLAGS=-g -L$(HOME)/lib $(patsubst %, -l%, $(DEPENDENCIES))

CXXFLAGS= -std=c++11  -DP_NEEDS_GNU_CXX_NAMESPACE=1 -DNDEBUG=1 -DNDEBUG  -g $(INCPATH)
ifdef DIST
	CXXFLAGS+=-O3	
endif 

COMMON_OBJECTS=$(COMMON_SOURCES:.cpp=.o)
DOCDUMP_OBJECTS=$(DOCDUMP_SOURCES:.cpp=.o)

all: makebmw

makebmw: $(DOCDUMP_TARGET)
	
$(DOCDUMP_TARGET): $(DOCDUMP_OBJECTS) $(COMMON_OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	rm -f $(DOCDUMP_TARGET) $(COMMON_OBJECTS) $(DOCDUMP_OBJECTS)

