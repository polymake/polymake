##
## Makefile for libnormaliz
##
include ../Makefile.configuration

LIBSOURCES = $(wildcard *.cpp)
LIBHEADERS = $(wildcard *.h)

CXXFLAGS += -I ..

default: libnormaliz.a

all: default

libnormaliz.o: $(LIBHEADERS) $(LIBSOURCES) libnormaliz-impl.cpp
	$(CXX) $(CXXFLAGS) $(NORMFLAGS) -c libnormaliz-impl.cpp -o libnormaliz.o

cone_property.o: cone_property.h cone_property.cpp
	$(CXX) $(CXXFLAGS) $(NORMFLAGS) -c cone_property.cpp -o $@

offload_handler.o: $(LIBHEADERS) offload_handler.cpp
	$(CXX) $(CXXFLAGS) $(NORMFLAGS) -c offload_handler.cpp -o $@

bottom.o: $(LIBHEADERS) bottom.cpp
	$(CXX) $(CXXFLAGS) $(NORMFLAGS) -c bottom.cpp -o $@

hilbert_series.o: $(LIBHEADERS) HilbertSeries.cpp
	$(CXX) $(CXXFLAGS) $(NORMFLAGS) -c HilbertSeries.cpp -o $@

libnormaliz-templated.o: $(LIBHEADERS) $(LIBSOURCES) libnormaliz-templated.cpp
	$(CXX) $(CXXFLAGS) $(NORMFLAGS) -c libnormaliz-templated.cpp -o $@

libnormaliz.a: cone_property.o hilbert_series.o libnormaliz-templated.o bottom.o offload_handler.o
	ar -cr $@ $^

clean:
	-rm -f *.o
	-rm -f libnormaliz.a

.PHONY : default clean all
