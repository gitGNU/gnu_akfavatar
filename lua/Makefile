CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = #-DLUA_USE_APICHECK

LUA_CFLAGS  := $(shell pkg-config lua5.1 --cflags)
LUA_LDFLAGS := $(shell pkg-config lua5.1 --libs)

srcdir = ..

prefix = /usr/local
luacdir = $(prefix)/lib/lua/5.1/
bindir = $(prefix)/bin

INSTALL = install

all: lua-akfavatar.so akfavatar-lua

# package
lua-akfavatar.so: lua-akfavatar.lo
	$(CC) $(CFLAGS) -fpic -shared -o $@ lua-akfavatar.lo \
	  -L. -lavtaddons -lakfavatar $(LDFLAGS)
# don't use LUA_LDFLAGS!

# standalone program
akfavatar-lua: akfavatar-lua.o lua-akfavatar.o
	$(CC) -o $@ akfavatar-lua.o lua-akfavatar.o \
	     -L. -lavtaddons -lakfavatar \
	     $(LUA_LDFLAGS) $(LDFLAGS)

lua-akfavatar.o: $(srcdir)/lua/lua-akfavatar.c
	$(CC) $(CFLAGS) $(LUA_CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/lua/lua-akfavatar.c

lua-akfavatar.lo: $(srcdir)/lua/lua-akfavatar.c
	$(CC) $(CFLAGS) $(LUA_CFLAGS) -I$(srcdir) -fpic -c -o $@ \
	  $(srcdir)/lua/lua-akfavatar.c

akfavatar-lua.o: $(srcdir)/lua/akfavatar-lua.c
	$(CC) $(CFLAGS) $(LUA_CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/lua/akfavatar-lua.c

install: lua-akfavatar.so akfavatar-lua
	-[ -d $(DESTDIR)$(luacdir) ] || $(INSTALL) -d $(DESTDIR)$(luacdir)
	$(INSTALL) -m 644 lua-akfavatar.so $(DESTDIR)$(luacdir)
	$(INSTALL) -m 755 akfavatar-lua $(DESTDIR)$(bindir)

clean:
	-rm -f lua-akfavatar.o lua-akfavatar.lo lua-akfavatar.wo
	-rm -f akfavatar-lua.o akfavatar-lua.wo
	-rm -f luac.out
	-rm -f lua-akfavatar.rc
	-rm -f *~

distclean: clean
	-rm -f akfavatar-lua
	-rm -f lua-akfavatar.so
	-rm -f akfavatar-lua.exe lua-akfavatar.dll
