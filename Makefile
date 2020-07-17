CFLAGS=-std=c11 -Wall -Wextra -pedantic -O2 -g

PROGRAMS = traceroute

all: $(PROGRAMS)

%: %.o
	$(CC) -o $@ $(LDFLAGS) $^ $(LDLIBS)

%.o: %.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

clean:
	$(RM) -fv $(PROGRAMS) $(PROGRAMS:=.o)

.PHONY: all clean
