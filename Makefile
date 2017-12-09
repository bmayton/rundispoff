all: rundispoff

rundispoff: rundispoff.c
	$(CC) $(CFLAGS) -o $@ $< -lX11 -lXext

clean:
	rm -f rundispoff

install: rundispoff
	install -m 755 rundispoff /usr/bin/rundispoff
