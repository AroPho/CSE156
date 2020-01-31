CXX := clang++ # set the compiler here
CXXFLAGS := -std=gnu++11 -Wall -Wextra -Wpedantic -Wshadow -g -Og # flags...
LDFLAGS := -lpthread

# List the programs in a variable so adding
# new programs is easier
PROGRAMS := myweb httpserver

all: $(PROGRAMS)

OBJECTS=$(PROGRAMS:.cpp=.o)

DEPS=$(PROGRAMS:.cpp=.d)

clean:
	-rm $(DEPS) $(PROGRAMS)

spotless: clean
	-rm $(PROGRAMS)

# no need to  write specific rules for
# your simple case where every program
# has a corresponding source code file
# of the same name and one file per program.

clean:
	rm -f *.o $(PROGRAMS)
