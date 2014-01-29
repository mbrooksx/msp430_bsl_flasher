CC=${CROSS_COMPILE}gcc
CFLAGS=-c -Wall 
LDFLAGS=-static
SOURCES=bsl_flasher.c lib_crc.c 
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=bsl_flasher

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *o bsl_flasher
