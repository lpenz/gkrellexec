
DESTDIR = /usr/local
PLUGINDIR = $(DESTDIR)/lib/gkrellm2/plugins

GTK2_INCLUDE = `pkg-config gtk+-2.0 --cflags`
GTK2_LIB = `pkg-config gtk+-2.0 --libs`

CPPFLAGS = -Wall -Werror
CFLAGS = -O2 -fPIC $(GTK2_INCLUDE)
LDLIBS = $(GTK2_LIB)

gkrellexec.so: gkrellexec.o

gkrellexec.o: gkrellexec.c

install: gkrellexec.so
	mkdir -p $(PLUGINDIR)
	install -c -s -m 644 gkrellexec.so $(PLUGINDIR)

clean:
	rm -f *.o *.so

%.so: %.o
	$(CC) -shared $(LDFLAGS) -o $@ $^ $(LOADLIBES) $(LDLIBS)
