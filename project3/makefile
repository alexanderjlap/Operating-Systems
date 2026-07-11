CCPP=g++
LIB=-lpthread -lrt

all: addem life

addem: addem.cpp
	$(CCPP) -o addem addem.cpp $(LIB)

life: life.cpp 
	$(CCPP) life.cpp -o life $(LIB)

clean:
	rm -f *.o *.txt addem life
