#ifndef sc_streamer_h
#define sc_streamer_h

#define SC_TimeBase    1000.0
#define SC_Reconnect_Attempts    200

#include <stdint.h>

#include "bytestream.h"
#include "flv_bytestream_ext.h"

typedef struct {
    flv_hnd_t *flv_out_handle;
    RTMP *rtmp;

    uint16_t frames;
    sc_time start_time_stamp;
    const char* stream_uri;
    const char* room_name;
    const char* so_name;
    
    sc_frame_rect capture_rect;

    x264_t* encoder;
    
    uint8_t rtmp_setup;
    uint8_t rtmpt;
    uint16_t so_version;
    uint8_t reconnect_tries;
} sc_streamer;

sc_streamer sc_streamer_init_video(const char* stream_host, const char* room_name, sc_frame_rect capture_rect, sc_time start_time_stamp);
sc_streamer sc_streamer_init_cursor(const char* stream_host, const char* room_name, sc_time start_time_stamp);
void sc_streamer_send_frame(sc_streamer *streamer, sc_frame *frame, sc_time frame_time_stamp);
void sc_streamer_send_mouse_data(sc_streamer *streamer, sc_mouse_coords *coords, sc_time coords_time_stamp);
void sc_streamer_stop_video(sc_streamer *streamer);
void sc_streamer_stop_cursor(sc_streamer *streamer);
void sc_streamer_setup_windows();
void sc_streamer_teardown_windows();

#endif
