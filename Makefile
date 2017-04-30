TARGET := lambda
SRC := lambda.cc parser.cc main.cc
HDR := lambda.h parser.h

CXXFLAGS += -std=c++11 -I/usr/local/include -g
LDFLAGS += -lboost_filesystem -lboost_system -L/usr/local/lib

$(TARGET): $(SRC) $(HDR)
	$(CXX) $(CXXFLAGS) $(SRC) $(LDFLAGS) -o $@

.phony: clean
clean:
	$(RM) $(TARGET)
