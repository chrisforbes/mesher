TARGET:=mesher
CXXSRCS:=$(shell find . -iname '*.cc')
CXXOBJS:=$(CXXSRCS:.cc=.o)

CXX:= g++

CXXFLAGS:=-O2 -pipe -W -Wall -Wextra -Werror
LDFLAGS:=-lshp -lm

$(TARGET): $(CXXOBJS) Makefile
	$(CXX) -o $@ $(CXXOBJS) $(LDFLAGS)

%.o: %.cc Makefile
	$(CXX) -o $@ -c $< $(CXXFLAGS)
