#ifndef sc_streamer_h
#define sc_streamer_h

#define SC_TimeBase    1000.0

#include <stdint.h>
// #include <math.h>

#include "bytestream.h"
#include "flv_bytestream_ext.h"

typedef struct {
    flv_hnd_t flv_out_handle;
    RTMP *rtmp;

    uint16_t frames;
    sc_time start_time_stamp;
    const char* stream_uri;
    const char* room_name;
    sc_frame_rect capture_rect;

    x264_t* encoder;
} sc_streamer;

sc_streamer sc_streamer_init(const char* stream_uri, const char* room_name, sc_frame_rect capture_rect, sc_time start_time_stamp);
void sc_streamer_send_frame(sc_streamer streamer, uint8_t* YUV_frame, sc_time frame_time_stamp);
void sc_streamer_send_mouse_data(sc_streamer streamer, sc_mouse_coords coords, sc_time coords_time_stamp);
void sc_streamer_stop(sc_streamer streamer);

#endif
