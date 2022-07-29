INCS := $(shell find ./ -name '*.h') $(shell find ../utilities/ -name '*.h')
SRCS := $(shell find ./ -name '*.cc') $(shell find ../utilities/ -name '*.cc')
OBJS := $(subst .cc,.o,$(SRCS))

CXX = g++
CXXFLAGS = -Wall -O2 -std=c++11 -I../utilities/

remoteClient: $(OBJS) $(INCS)
	@$(CXX) $(CXXFLAGS) $(OBJS) -o remoteClient

.SILENT: $(OBJS)

%.o: %.cc
	@$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean

clean:
	@echo "Cleaning up (client)..."
	@rm -f $(OBJS) ./remoteClient
