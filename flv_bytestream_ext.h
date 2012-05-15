#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <librtmp/rtmp.h>
#include <x264cli.h>
#include <output/flv_bytestream.h>

typedef struct
{
    flv_buffer *c;

    uint8_t *sei;
    int sei_len;

    int64_t i_fps_num;
    int64_t i_fps_den;
    int64_t i_framenum;

    uint64_t i_framerate_pos;
    uint64_t i_duration_pos;
    uint64_t i_filesize_pos;
    uint64_t i_bitrate_pos;

    uint8_t b_write_length;
    int64_t i_prev_dts;
    int64_t i_prev_cts;
    int64_t i_delay_time;
    int64_t i_init_delta;
    int i_delay_frames;

    double d_timebase;
    int b_vfr_input;
    int b_dts_compress;

    unsigned start;
} flv_hnd_t;

flv_buffer *flv_create_RTMP_writer( const char *stream_uri, RTMP *rtmp );
int flv_close_RTMP_writer( RTMP *rtmp );
int flv_flush_RTMP_data( RTMP *rtmp, flv_buffer *c );
RTMP *open_RTMP_stream( const char *stream_uri, flv_hnd_t *p_handle );
int send_invoke( RTMP *rtmp, uint16_t x, uint16_t y, uint32_t timestamp, const char* room_name );
int close_RTMP_stream(flv_hnd_t handle, RTMP *rtmp);

int set_param( flv_hnd_t handle, x264_param_t *p_param );
int write_headers( flv_hnd_t handle, x264_nal_t *p_nal );
int write_frame( flv_hnd_t handle, uint8_t *p_nalu, int i_size, x264_picture_t *p_picture );
