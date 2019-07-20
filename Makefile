TARGET=tcpfwd
LIBS=
CC=gcc
CFLAGS=-Wall $(DEBUG)

#https://www.gnu.org/software/make/manual/make.html#Wildcard-Function
_objects=$(patsubst %.c,%.o,$(wildcard *.c))


default:
	@echo "Specify target! (or make all)"

all: $(TARGET)

%.o: %.c %.h
	$(CC) -c $< $(CFLAGS) -o $@

$(TARGET): $(_objects)
	$(CC) -o $(TARGET) $(_objects)

clean:
	rm *.o $(TARGET)
