CXX = g++
CXXFLAGS = -Wall -std=c++17
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

all: example_game

example_game: example_game.cpp GameEngine.hpp
	$(CXX) $(CXXFLAGS) example_game.cpp -o example_game $(LDFLAGS)

clean:
	rm -f example_game
