CXX = g++
CXXFLAGS = -O3 -Wall -std=c++17
LDFLAGS = -lfmt
INCLUDE_DIRS = -I/usr/include
LIB_DIRS = -L/usr/lib

SRC = main.cpp
OBJ = $(SRC:.cpp=.o)
EXE = gentoofetch

all: $(EXE)

$(EXE): $(OBJ)
	$(CXX) $(OBJ) -o $(EXE) $(LIB_DIRS) $(LDFLAGS)

$(OBJ): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $(SRC)

install: $(EXE)
	install -m 755 $(EXE) /usr/local/bin/

clean:
	rm -f $(OBJ) $(EXE)

