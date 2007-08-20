# This file is in the Public Domain - AKFoerster

include version

TARGET = i586-mingw32msvc
CC = $(TARGET)-gcc
AR = $(TARGET)-ar
WINDRES = $(TARGET)-windres
RANLIB = $(TARGET)-ranlib
CFLAGS = -O2 -DNOFIFO
LDFLAGS = 
SDLCFLAGS = -I/usr/$(TARGET)/include/SDL/ \
         -D_GNU_SOURCE -Dmain=SDL_main
SDLLDFLAGS = -L/usr/$(TARGET)/lib/ -lmingw32 -lSDLmain -lSDL -mwindows

# native charset encoding (defaults to "WCHAR_T")
ICONV_TARGET = -DICONV_TARGET=\"UCS-2\"

prefix = /usr/$(TARGET)
libdir = $(prefix)/lib
includedir = $(prefix)/include

all: libavatar.a avatar.dll avatarsay.exe

avtsayinfo.rc:
	@echo "AppIcon ICON \"akfavatar.ico\"" > $@
	@echo >> $@
	@echo "1 VERSIONINFO" >> $@
	@echo "FILEVERSION $(VERSION), $(SUBVERSION), 0, 0" >> $@
	@echo "PRODUCTVERSION $(VERSION), $(SUBVERSION), 0, 0" >> $@
	@echo "FILEFLAGSMASK 0x3F" >> $@
	@echo "FILEOS 0x4" >> $@
	@echo "FILETYPE 0x1" >> $@
	@echo "BEGIN" >> $@
	@echo " BLOCK \"StringFileInfo\"" >> $@
	@echo " BEGIN" >> $@
	@echo "  BLOCK \"040904E4\"" >> $@
	@echo "  BEGIN" >> $@
	@echo "   VALUE \"CompanyName\", \"AKFoerster\"" >> $@
	@echo "   VALUE \"FileDescription\", \"fancy text-viewer and scripting language\"" >> $@
	@echo "   VALUE \"FileVersion\", \"$(VERSION).$(SUBVERSION)$(VARIANT)\"" >> $@
	@echo "   VALUE \"InternalName\", \"avatarsay\"" >> $@
	@echo "   VALUE \"LegalCopyright\", \"GPLv3+, Andreas K. F\366rster\"" >> $@
	@echo "   VALUE \"OriginalFilename\", \"avatarsay.exe\"" >> $@
	@echo "   VALUE \"Comments\", \"primarily written for GNU/Linux\"" >> $@
	@echo "   VALUE \"ProductName\", \"AKFAvatar\"" >> $@
	@echo "   VALUE \"ProductVersion\", \"$(VERSION).$(SUBVERSION)$(VARIANT)\"" >> $@
	@echo "  END" >> $@
	@echo " END" >> $@
	@echo "END" >> $@

libinfo.rc:
	@echo "1 VERSIONINFO" > $@
	@echo "FILEVERSION $(VERSION), $(SUBVERSION), 0, 0" >> $@
	@echo "PRODUCTVERSION $(VERSION), $(SUBVERSION), 0, 0" >> $@
	@echo "FILEFLAGSMASK 0x3F" >> $@
	@echo "FILEOS 0x4" >> $@
	@echo "FILETYPE 0x1" >> $@
	@echo "BEGIN" >> $@
	@echo " BLOCK \"StringFileInfo\"" >> $@
	@echo " BEGIN" >> $@
	@echo "  BLOCK \"040904E4\"" >> $@
	@echo "  BEGIN" >> $@
	@echo "   VALUE \"CompanyName\", \"AKFoerster\"" >> $@
	@echo "   VALUE \"FileDescription\", \"Avatar library\"" >> $@
	@echo "   VALUE \"FileVersion\", \"$(VERSION).$(SUBVERSION)$(VARIANT)\"" >> $@
	@echo "   VALUE \"InternalName\", \"avatar\"" >> $@
	@echo "   VALUE \"LegalCopyright\", \"GPLv3+, Andreas K. F\366rster\"" >> $@
	@echo "   VALUE \"OriginalFilename\", \"avatar.dll\"" >> $@
	@echo "   VALUE \"Comments\", \"primarily written for GNU/Linux\"" >> $@
	@echo "   VALUE \"ProductName\", \"AKFAvatar\"" >> $@
	@echo "   VALUE \"ProductVersion\", \"$(VERSION).$(SUBVERSION)$(VARIANT)\"" >> $@
	@echo "  END" >> $@
	@echo " END" >> $@
	@echo "END" >> $@

icon.o: icon.rc akfavatar.ico
	$(WINDRES) -o $@ -i $<

avtsayinfo.o: avtsayinfo.rc akfavatar.ico
	$(WINDRES) -o $@ -i $<

libinfo.o: libinfo.rc
	$(WINDRES) -o $@ -i $<

avatar.o: ../avatar.c ../avatar.h ../ucsfont7x14.c
	$(CC) $(CFLAGS) $(SDLCFLAGS) $(ICONV_TARGET) -c $<

%.o: ../%.c
	$(CC) -c $(CFLAGS) $(SDLCFLAGS) -o $@ $<

# static library
libavatar.a: avatar.o avatar-default.o avatar-audio.o
	$(AR) rcu $@ avatar.o avatar-default.o avatar-audio.o
	$(RANLIB) $@

avatar.dll: avatar.o avatar-default.o avatar-audio.o libinfo.o
	$(CC) -shared -o avatar.dll avatar.o avatar-default.o avatar-audio.o \
	        libinfo.o \
		-Wl,-no-undefined,--enable-runtime-pseudo-reloc \
		-Wl,--output-def,avatar.def,--out-implib,libavatar.dll.a \
		$(LDFLAGS) $(SDLLDFLAGS)

install: avatar.dll libavatar.a
	install -m 644 ../avatar.h ../avatar-audio.h $(includedir)
	install -m 644 libavatar.a libavatar.dll.a $(libdir)
	install -m 644 avatar.dll $(libdir)

install-strip: avatar.dll libavatar.a
	install -m 644 ../avatar.h ../avatar-audio.h $(includedir)
	install -m 644 libavatar.a libavatar.dll.a $(libdir)
	install -m 644 -s avatar.dll $(libdir)

example.exe: ../example.c avatar.dll icon.o
	$(CC) $(CFLAGS) $(SDLCFLAGS) -o $@ $< icon.o \
		-L. -lavatar $(LDFLAGS) $(SDLLDFLAGS)

# uses static lib
avatarsay.exe: ../avatarsay.c libavatar.a avtsayinfo.o
	$(CC) $(CFLAGS) $(SDLCFLAGS) -o $@ $< avtsayinfo.o \
		-L. libavatar.a $(LDFLAGS) $(SDLLDFLAGS)

clean:
	-rm -f *~ *.o 
	-rm -f libinfo.rc avtsayinfo.rc

distclean: clean
	-rm -f *.a *.so *.exe avatarsay-*.zip
	-rm -f avatar.def avatar.dll avatar.lib
	-rm -f version

dist-bin: avatar.dll avatarsay.exe
	mkdir avatarsay-$(VERSION).$(SUBVERSION)$(VARIANT)
	cp avatarsay.exe ../fsdemo-?? ../avtcharset \
	   avatarsay-$(VERSION).$(SUBVERSION)$(VARIANT)/
	cp fsdemo-??.bat avtcharset.bat \
	   avatarsay-$(VERSION).$(SUBVERSION)$(VARIANT)/
	cp ../COPYING avatarsay-$(VERSION).$(SUBVERSION)$(VARIANT)/gpl-3.0.txt
	strip avatarsay-$(VERSION).$(SUBVERSION)$(VARIANT)/avatarsay.exe
	todos avatarsay-$(VERSION).$(SUBVERSION)$(VARIANT)/fsdemo-??
	todos avatarsay-$(VERSION).$(SUBVERSION)$(VARIANT)/avtcharset
	todos avatarsay-$(VERSION).$(SUBVERSION)$(VARIANT)/*.bat
	todos avatarsay-$(VERSION).$(SUBVERSION)$(VARIANT)/gpl-3.0.txt
	zip -9vr avatarsay-$(VERSION).$(SUBVERSION)$(VARIANT).bin.w32.zip \
	    avatarsay-$(VERSION).$(SUBVERSION)$(VARIANT)/
	rm -r avatarsay-$(VERSION).$(SUBVERSION)$(VARIANT)/
