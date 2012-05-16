// bytestream.c
//
// Author:  Luke van der Hoeven
// Date:    7/5/12

#include <time.h>
#include "bytestream.h"

sc_bytestream_header create_header(uint8_t type) {
  sc_bytestream_header header = {(uint16_t)type, time(NULL)};
  return header;
}

tpl_bin create_body(void *addr, int sz) {
  tpl_bin body;

  body.addr = addr;
  body.sz = sz;

  return body;
}

sc_bytestream_packet sc_bytestream_put_start(int fd) {
  sc_bytestream_packet packet = {create_header(START), create_body(NULL, 0)};

  serialize_packet(fd, packet);
  return packet;
}

sc_bytestream_packet sc_bytestream_put_stop(int fd) {
  sc_bytestream_packet packet = {create_header(STOP), create_body(NULL, 0)};

  serialize_packet(fd, packet);
  return packet;
}

sc_bytestream_packet sc_bytestream_put_mouse_data(int fd, sc_mouse_coords coords) {
  sc_bytestream_packet packet = {create_header(MOUSE), create_body(&coords, sizeof(coords))};
  serialize_packet(fd, packet);

  return packet;
}

sc_mouse_coords sc_bytestream_get_mouse_data(int fd) {
  sc_bytestream_packet packet = deserialize_packet(fd);

  return parse_mouse_coords(packet);
}

sc_bytestream_packet sc_bytestream_put_frame(int fd, sc_frame frame) {
  sc_bytestream_packet packet = {create_header(VIDEO), create_body(&frame, sizeof(frame))};
  serialize_packet(fd, packet);

  return packet;
}

sc_frame sc_bytestream_get_frame(int fd) {
  sc_bytestream_packet packet = deserialize_packet(fd);

  return parse_frame(packet);
}

sc_bytestream_packet sc_bytestream_get_event(int fd) {
  sc_bytestream_packet packet = deserialize_packet(fd);
  return packet;
}

sc_bytestream_header sc_bytestream_get_event_header(int fd) {
  sc_bytestream_packet packet = sc_bytestream_get_event(fd);
  return packet.header;
}

// ENCODING/DECODING HELPERS

sc_mouse_coords parse_mouse_coords(sc_bytestream_packet packet) {
  void *data = packet.body.addr;

  sc_mouse_coords coords = *((sc_mouse_coords *) data);
  free(data);

  return coords;
}

sc_frame parse_frame(sc_bytestream_packet packet) {
  void *data = packet.body.addr;

  sc_frame frame = *((sc_frame *) data);
  free(data);

  return frame;
}

void serialize_packet(int fd, sc_bytestream_packet packet) {
  tpl_node *tn = tpl_map(TPL_STRUCTURE, &packet.header, &packet.body);
  tpl_pack(tn, 0);
  tpl_dump(tn, TPL_FD, fd);
  tpl_free(tn);
}

sc_bytestream_packet deserialize_packet(int fd) {
  sc_bytestream_packet packet;
  sc_bytestream_header header;
  tpl_bin data;

  tpl_node *tn = tpl_map(TPL_STRUCTURE, &header, &data);
  int loaded = tpl_load(tn, TPL_FD, fd);

  if( loaded == -1 ) {
    packet.header = create_header(NO_DATA);
    packet.body = create_body(NULL, 0);
  } else {
    tpl_unpack(tn, 0);
    packet.header = header;
    packet.body = data;
  }

  tpl_free(tn);
  free(data.addr);

  return packet;
}
