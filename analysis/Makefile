# Simple makefile

EXE=doAnalysis

SOURCES=$(wildcard *.cc)
OBJECTS=$(SOURCES:.cc=.o)
HEADERS=$(SOURCES:.cc=.h)

CC         = g++
CXX        = g++
DEBUG      = -g
#DEBUG      =
CXXFLAGS   = $(DEBUG) -O2 -Wall -fPIC -Wshadow -Woverloaded-virtual
LD         = g++
LDFLAGS    = $(DEBUG) -O2
SOFLAGS    = $(DEBUG) -shared
CXXFLAGS   = $(DEBUG) -O2 -Wall -fPIC -Wshadow -Woverloaded-virtual
LDFLAGS    = $(DEBUG) -O2
ROOTLIBS   = $(shell root-config --libs)
ROOTCFLAGS = $(shell root-config --cflags)
CXXFLAGS  += $(ROOTCFLAGS)
CFLAGS     = $(ROOTCFLAGS) -Wall -Wno-unused-function $(DEBUG) -O2 -fPIC -fno-var-tracking
EXTRAFLAGS = -fPIC -ITMultiDrawTreePlayer -Wunused-variable -lTMVA -lEG -lGenVector -lXMLIO -lMLP -lTreePlayer

#.PHONY: check-env

$(EXE): $(OBJECTS) wwwtree.o
	$(LD) $(CXXFLAGS) $(LDFLAGS) $(OBJECTS) $(ROOTLIBS) $(EXTRAFLAGS) -o $@

%.o: %.cc
	$(CC) $(CFLAGS) $< -c

clean:
	rm -f *.o $(EXE)
