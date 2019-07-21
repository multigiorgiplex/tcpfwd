TARGET=tcpfwd
LIBS=
CC=gcc
STRIP=strip
CFLAGS=-Wall $(DEBUG)

#https://www.gnu.org/software/make/manual/make.html#Wildcard-Function
_objects=$(patsubst %.c,%.o,$(wildcard *.c))


default:
	@echo "Specify target! (or make all)"

all: $(TARGET)

debug: DEBUG=-g3
debug: $(TARGET)

release: DEBUG=-g0
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
