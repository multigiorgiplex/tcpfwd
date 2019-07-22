TARGET=tcpfwd
LIBS=
CC=gcc
STRIP=strip
CFLAGS=$(WARNINGS) $(DEBUG)

#https://www.gnu.org/software/make/manual/make.html#Wildcard-Function
_objects=$(patsubst %.c,%.o,$(wildcard *.c))


default:
	@echo "Specify target! (or make all)"

all: $(TARGET)

debug: CFLAGS=-g3
debug: $(TARGET)

release: CFLAGS=-g0 -Wall
release: $(TARGET)
	$(STRIP) $(TARGET)

%.o: %.c %.h
	$(CC) -c $< $(CFLAGS) $(LIBS) -o $@

$(TARGET): $(_objects)
	$(CC) -o $(TARGET) $(_objects)

clean:
	rm -vf *.o
	rm -vf *.gch
	rm -vf $(TARGET)
