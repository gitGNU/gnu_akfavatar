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

# name for binary packages
BINPKGNAME=akfavatar-$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT).bin

prefix = /usr/$(TARGET)
libdir = $(prefix)/lib
includedir = $(prefix)/include

all: akfavatar.dll avatarsay.exe

avtsayinfo.rc:
	@echo "AppIcon ICON \"akfavatar.ico\"" > $@
	@echo >> $@
	@echo "1 VERSIONINFO" >> $@
	@echo "FILEVERSION $(VERSION), $(SUBVERSION), $(REVISION), 0" >> $@
	@echo "PRODUCTVERSION $(VERSION), $(SUBVERSION), $(REVISION), 0" >> $@
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
	@echo "   VALUE \"FileVersion\", \"$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT)\"" >> $@
	@echo "   VALUE \"InternalName\", \"avatarsay\"" >> $@
	@echo "   VALUE \"LegalCopyright\", \"GPLv3+, Andreas K. F\366rster\"" >> $@
	@echo "   VALUE \"OriginalFilename\", \"avatarsay.exe\"" >> $@
	@echo "   VALUE \"Comments\", \"primarily written for GNU/Linux\"" >> $@
	@echo "   VALUE \"ProductName\", \"AKFAvatar\"" >> $@
	@echo "   VALUE \"ProductVersion\", \"$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT)\"" >> $@
	@echo "  END" >> $@
	@echo " END" >> $@
	@echo "END" >> $@

libinfo.rc:
	@echo "1 VERSIONINFO" > $@
	@echo "FILEVERSION $(VERSION), $(SUBVERSION), $(REVISION), 0" >> $@
	@echo "PRODUCTVERSION $(VERSION), $(SUBVERSION), $(REVISION), 0" >> $@
	@echo "FILEFLAGSMASK 0x3F" >> $@
	@echo "FILEOS 0x4" >> $@
	@echo "FILETYPE 0x1" >> $@
	@echo "BEGIN" >> $@
	@echo " BLOCK \"StringFileInfo\"" >> $@
	@echo " BEGIN" >> $@
	@echo "  BLOCK \"040904E4\"" >> $@
	@echo "  BEGIN" >> $@
	@echo "   VALUE \"CompanyName\", \"AKFoerster\"" >> $@
	@echo "   VALUE \"FileDescription\", \"AKFAvatar library\"" >> $@
	@echo "   VALUE \"FileVersion\", \"$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT)\"" >> $@
	@echo "   VALUE \"InternalName\", \"akfavatar\"" >> $@
	@echo "   VALUE \"LegalCopyright\", \"GPLv3+, Andreas K. F\366rster\"" >> $@
	@echo "   VALUE \"OriginalFilename\", \"akfavatar.dll\"" >> $@
	@echo "   VALUE \"Comments\", \"primarily written for GNU/Linux\"" >> $@
	@echo "   VALUE \"ProductName\", \"AKFAvatar\"" >> $@
	@echo "   VALUE \"ProductVersion\", \"$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT)\"" >> $@
	@echo "  END" >> $@
	@echo " END" >> $@
	@echo "END" >> $@

icon.o: icon.rc akfavatar.ico
	$(WINDRES) -o $@ -i $<

avtsayinfo.o: avtsayinfo.rc akfavatar.ico
	$(WINDRES) -o $@ -i $<

libinfo.o: libinfo.rc
	$(WINDRES) -o $@ -i $<

avatar.o: ../avatar.c ../akfavatar.h ../ucsfont7x14.c
	$(CC) $(CFLAGS) $(SDLCFLAGS) $(ICONV_TARGET) -c $<

%.o: ../%.c
	$(CC) -c $(CFLAGS) $(SDLCFLAGS) -o $@ $<

# static library
libakfavatar.a: avatar.o avatar-default.o avatar-audio.o
	$(AR) rcu $@ avatar.o avatar-default.o avatar-audio.o
	$(RANLIB) $@

akfavatar.dll: avatar.o avatar-default.o avatar-audio.o libinfo.o
	$(CC) -shared -o $@ avatar.o avatar-default.o avatar-audio.o \
	        libinfo.o \
		-Wl,-no-undefined,--enable-runtime-pseudo-reloc \
	        -Wl,--output-def,akfavatar.def \
	        -Wl,--out-implib,libakfavatar.dll.a \
		$(LDFLAGS) $(SDLLDFLAGS)

install: akfavatar.dll libakfavatar.a
	install -m 644 ../akfavatar.h $(includedir)
	install -m 644 libakfavatar.a libakfavatar.dll.a $(libdir)
	install -m 644 akfavatar.dll $(libdir)

install-strip: akfavatar.dll libakfavatar.a
	install -m 644 ../akfavatar.h $(includedir)
	install -m 644 libakfavatar.a libakfavatar.dll.a $(libdir)
	install -m 644 -s akfavatar.dll $(libdir)

example: example.exe

example.exe: ../example.c akfavatar.dll icon.o
	$(CC) $(CFLAGS) $(SDLCFLAGS) -o $@ $< icon.o \
			-L. -lakfavatar $(LDFLAGS) $(SDLLDFLAGS)

avatarsay: avatarsay.exe

# uses dynamic lib
avatarsay.exe: ../avatarsay.c akfavatar.dll avtsayinfo.o
	$(CC) $(CFLAGS) $(SDLCFLAGS) -o $@ $< avtsayinfo.o \
		-L. -lakfavatar $(LDFLAGS) $(SDLLDFLAGS)

clean:
	-rm -f *~ *.o *.ppu *.gpi
	-rm -f libinfo.rc avtsayinfo.rc

distclean: clean
	-rm -f *.a *.so *.exe avatarsay-*.zip
	-rm -f akfavatar.def akfavatar.dll akfavatar.lib
	-rm -f version


# the folowing targets are for the maintainer
# don't use them unless you know what you are doing

multiply.exe: ../pascal/multiply.pas
	fpcwavatar -dRELEASE -FE. -o$@ $<

multiplizieren.exe: ../pascal/multiply.pas
	fpcwavatar -dRELEASE -ddeutsch -FE. -o$@ $<

dist-bin: avatarsay.exe multiply.exe multiplizieren.exe
	mkdir $(BINPKGNAME)
	cp akfavatar.dll $(BINPKGNAME)/
	cp avatarsay.exe ../fsdemo-?? $(BINPKGNAME)/
	cp fsdemo-??.bat $(BINPKGNAME)/
	cp ../COPYING $(BINPKGNAME)/gpl-3.0.txt
	cp ../AUTHORS $(BINPKGNAME)/AUTHORS.txt
	cp ../pascal/correct.wav ../pascal/wrong.wav $(BINPKGNAME)/
	cp multiply.exe multiplizieren.exe $(BINPKGNAME)/
	strip $(BINPKGNAME)/*.exe $(BINPKGNAME)/akfavatar.dll
	todos $(BINPKGNAME)/fsdemo-??
	todos $(BINPKGNAME)/avtcharset
	todos $(BINPKGNAME)/*.bat
	todos $(BINPKGNAME)/*.txt
	-rm -f $(BINPKGNAME).w32.zip
	zip -9vr $(BINPKGNAME).w32.zip $(BINPKGNAME)/
	rm -r $(BINPKGNAME)/
