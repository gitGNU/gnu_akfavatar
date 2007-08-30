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

all: libakfavatar.a akfavatar.dll avatarsay.exe

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
	@echo "   VALUE \"FileDescription\", \"Avatar library\"" >> $@
	@echo "   VALUE \"FileVersion\", \"$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT)\"" >> $@
	@echo "   VALUE \"InternalName\", \"avatar\"" >> $@
	@echo "   VALUE \"LegalCopyright\", \"GPLv3+, Andreas K. F\366rster\"" >> $@
	@echo "   VALUE \"OriginalFilename\", \"avatar.dll\"" >> $@
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

# uses static lib
avatarsay.exe: ../avatarsay.c libakfavatar.a avtsayinfo.o
	$(CC) $(CFLAGS) $(SDLCFLAGS) -o $@ $< avtsayinfo.o \
		-L. libakfavatar.a $(LDFLAGS) $(SDLLDFLAGS)

clean:
	-rm -f *~ *.o 
	-rm -f libinfo.rc avtsayinfo.rc

distclean: clean
	-rm -f *.a *.so *.exe avatarsay-*.zip
	-rm -f akfavatar.def akfavatar.dll akfavatar.lib
	-rm -f version

dist-bin: avatarsay.exe
	mkdir avatarsay-$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT)
	cp avatarsay.exe ../fsdemo-?? \
	   avatarsay-$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT)/
	cp fsdemo-??.bat \
	   avatarsay-$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT)/
	cp ../COPYING avatarsay-$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT)/gpl-3.0.txt
	strip avatarsay-$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT)/avatarsay.exe
	todos avatarsay-$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT)/fsdemo-??
	todos avatarsay-$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT)/avtcharset
	todos avatarsay-$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT)/*.bat
	todos avatarsay-$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT)/gpl-3.0.txt
	zip -9vr avatarsay-$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT).bin.w32.zip \
	    avatarsay-$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT)/
	rm -r avatarsay-$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT)/
