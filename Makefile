LIB = lib
X264 = libx264
RTMP = librtmp/librtmp

# Flags passed to the preprocessor.
CPPFLAGS += -I$(X264) -I$(RTMP) -I$(LIB)

# Flags passed to the C++ compiler.
CXXFLAGS += -s -Os -g -Wall -Wextra -std=c99

CXX = gcc

#See if we are building on Windows
ifeq ($(OS),Windows_NT)
	BUILD = screen_streamer.exe
	SYS = mingw
else
	BUILD = screen_streamer
	SYS = posix
	CXXFLAGS += -arch x86_64 -arch i386
endif

# House-keeping build targets.

all : $(BUILD)

clean :
	rm -f $(BUILD) *.o

$(RTMP)/librtmp.a :
	@cd $(RTMP) && $(MAKE) SHARED= SYS=$(SYS) CRYPTO=
		
$(X264)/libx264.a :
	@cd $(X264) && ./configure --disable-cli --enable-static --enable-strip && $(MAKE)

install :
ifeq ($(OS),Windows_NT)
	cp screen_streamer.exe ../screenshare/ScreenShareWindowsClient/Additional\ Release\ Files
	cp screen_streamer.exe ../screenshare/ScreenShareWindowsClient/Debug
	cp screen_streamer.exe ../screenshare/ScreenShareWindowsClient/Release
else 
	cp screen_streamer ../screenshare/ScreenShareOSXClient
endif
 
mmap.o : $(LIB)/win/mmap.c
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(LIB)/win/mmap.c

tpl.o : $(LIB)/tpl.c
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(LIB)/tpl.c

flv_bytestream.o : $(X264)/output/flv_bytestream.c
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(X264)/output/flv_bytestream.c

flv_bytestream_ext.o : flv_bytestream_ext.c
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c flv_bytestream_ext.c

bytestream.o : $(LIB)/bytestream.c
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(LIB)/bytestream.c

sc_streamer.o : $(LIB)/bytestream.h $(LIB)/tpl.h sc_streamer.h sc_streamer.c
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c sc_streamer.c

screen_streamer : tpl.o flv_bytestream_ext.o bytestream.o flv_bytestream.o sc_streamer.o $(RTMP)/librtmp.a $(X264)/libx264.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -lpthread -o $@

screen_streamer.exe : mmap.o tpl.o flv_bytestream_ext.o bytestream.o flv_bytestream.o sc_streamer.o $(RTMP)/librtmp.a $(X264)/libx264.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -lws2_32 -lpthread -lwinmm -o $@
