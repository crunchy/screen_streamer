# Flags passed to the preprocessor.
CPPFLAGS += -I/Users/hungerandthirst/Code/Projects/crunchy/screenshare/screen_streamer/x264 \
	-I/Users/hungerandthirst/Code/Projects/crunchy/screenshare/screen_streamer/librtmp \
	-I/Users/hungerandthirst/Code/Projects/crunchy/screenshare/screen_streamer/lib

# Flags passed to the C++ compiler.
CXXFLAGS += -g -Wall -Wextra -std=c99 -arch x86_64

CXX = gcc

LIB = ./lib

BUILD = screen_streamer

# House-keeping build targets.

all : $(BUILD)

clean :
	rm -f $(BUILD) *.o

tpl.o : $(LIB)/tpl.c
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(LIB)/tpl.c

bytestream.o : $(LIB)/bytestream.c $(LIB)/tpl.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(LIB)/bytestream.c

sc_streamer.o : $(LIB)/bytestream.h $(LIB)/tpl.h sc_streamer.h sc_streamer.c
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c sc_streamer.c

screen_streamer : bytestream.o sc_streamer.o tpl.o librtmp.a x264/libx264.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@
