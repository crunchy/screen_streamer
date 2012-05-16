LIB = lib
X264 = libx264
RTMP = librtmp/librtmp

# Flags passed to the preprocessor.
CPPFLAGS += -I$(X264) -I$(RTMP) -I$(LIB)

# Flags passed to the C++ compiler.
CXXFLAGS += -g -Wall -Wextra -std=c99 -arch x86_64

CXX = gcc


BUILD = screen_streamer

# House-keeping build targets.

all : $(BUILD)

clean :
	rm -f $(BUILD) *.o

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

screen_streamer : flv_bytestream_ext.o bytestream.o flv_bytestream.o sc_streamer.o tpl.o $(RTMP)/librtmp.a $(X264)/libx264.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@