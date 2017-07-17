DOCDUMP_TARGET=CentroidDistancesOld
COMMON_SOURCES=
DOCDUMP_SOURCES=CentroidDistances.cpp
CXX=g++

ifdef OLDINDRI
	DEPENDENCIES = lemur xpdf antlr boost_filesystem boost_system
	INCPATH=-I$(HOME)/indri-5.2/include $(patsubst %, -I$(HOME)/indri-5.2/contrib/%/include, $(DEPENDENCIES))   -I$(HOME)/include
	LDFLAGS=-g -L$(HOME)/indri-5.2/obj $(patsubst %, -L$(HOME)/indri-5.2/contrib/%/obj, $(DEPENDENCIES)) -L$(HOME)/opt/db-6.0.20.NC/build_unix -L$(HOME)/lib -ldb_cxx -lindri $(patsubst %, -l%, $(DEPENDENCIES)) -lz -lpthread -lm
else
	DEPENDENCIES = db_cxx indri z pthread m boost_filesystem boost_system
	INCPATH=-I$(HOME)/include 
	LDFLAGS=-g -L$(HOME)/lib $(patsubst %, -l%, $(DEPENDENCIES))
endif

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

