// bytestream.h
//
// Purpose: Utilities for creating bytestream-able data
// Author:  Luke van der Hoeven
// Date:    7/5/12

// Protocol definition

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "tpl.h"

#define SC_TimeBase    1000.0

#define STOP  0x01
#define START 0x02
#define MOUSE 0x03
#define VIDEO 0x04
#define NO_DATA 0xFF

typedef uint32_t sc_time;
static char TPL_STRUCTURE[] = "S(vu)B";

typedef struct {
    uint16_t x, y;
} sc_mouse_coords;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} sc_frame_rect;

typedef struct {
  uint16_t type;
  sc_time timestamp;
} sc_bytestream_header;

typedef struct {
  uint8_t *framePtr;
  int size;
} sc_frame;

typedef struct {
  sc_bytestream_header header;
  tpl_bin body;
} sc_bytestream_packet;

sc_bytestream_packet sc_bytestream_put_start(int fd);
sc_bytestream_packet sc_bytestream_put_stop(int fd);
sc_bytestream_packet sc_bytestream_put_mouse_data(int fd, sc_mouse_coords coords);
sc_bytestream_packet sc_bytestream_put_frame(int fd, sc_frame frame);

sc_mouse_coords sc_bytestream_get_mouse_data(int fd);
sc_frame sc_bytestream_get_frame(int fd);

sc_bytestream_packet sc_bytestream_get_event(int fd);
sc_bytestream_header sc_bytestream_get_event_header(int fd);

sc_bytestream_header create_header(uint8_t event);
void serialize_packet(int fd, sc_bytestream_packet packet);
sc_bytestream_packet deserialize_packet(int fd);
sc_frame parse_frame(sc_bytestream_packet packet);
sc_mouse_coords parse_mouse_coords(sc_bytestream_packet packet);
