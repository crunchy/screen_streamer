#include "flv_bytestream_ext.h"

#define CHECK(x)\
do {\
    if( (x) < 0 )\
        return -1;\
} while( 0 )

RTMP *open_RTMP_stream(const char *stream_uri, flv_hnd_t *p_handle)
{
    flv_hnd_t *p_flv = malloc( sizeof(*p_flv) );
    p_handle = NULL;

    if( !p_flv )
        return NULL;
    memset( p_flv, 0, sizeof(*p_flv) );

    RTMP *rtmp;
    p_flv->c = flv_create_RTMP_writer( stream_uri, rtmp );
    if( !p_flv->c )
        return NULL;

    p_handle = p_flv;

    return rtmp;
}

int close_RTMP_stream(flv_hnd_t handle, RTMP *rtmp)
{
    flv_hnd_t *p_flv = &handle;
    flv_buffer *c = p_flv->c;

    flv_close_RTMP_writer( rtmp );

    fclose( c->fp );
    free( p_flv );
    free( c );

    return 0;
}

int send_invoke( RTMP *rtmp, uint16_t x, uint16_t y, uint32_t timestamp, const char *room_name )
{
  RTMPPacket packet;
  AVal name = {room_name, strlen(room_name)};
  AVal command = AVC("updateCursorLocation");

  char pbuffer[512], *pend = pbuffer + sizeof(pbuffer);
  char *enc;

  packet.m_nChannel = 0x03;
  packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuffer + RTMP_MAX_HEADER_SIZE;

  enc = packet.m_body;
  enc = AMF_EncodeString(enc, pend, &command);
  enc = AMF_EncodeNumber(enc, pend, ++(rtmp->m_numInvokes));
  *enc++ = AMF_NULL;

  enc = AMF_EncodeNumber(enc, pend, x);
  enc = AMF_EncodeNumber(enc, pend, y);
  enc = AMF_EncodeNumber(enc, pend, timestamp);
  enc = AMF_EncodeString(enc, pend, &name);

  packet.m_nBodySize = enc - packet.m_body;

  return RTMP_SendPacket(rtmp, &packet, TRUE);
}

int flv_close_RTMP_writer( RTMP *rtmp )
{
    RTMP_Close(rtmp);
    RTMP_Free(rtmp);

    return 0;
}

int flv_flush_RTMP_data( RTMP *rtmp, flv_buffer *flv )
{
    if( !flv->d_cur )
        return 0;

    RTMP_Write(rtmp, (char *)flv->data, flv->d_cur);

    flv->d_total += flv->d_cur;

    flv->d_cur = 0;

    return 0;
}

flv_buffer *flv_create_RTMP_writer( const char *stream_uri, RTMP *rtmp )
{
    flv_buffer *c = malloc( sizeof(*c) );

    if( !c )
        return NULL;
    memset( c, 0, sizeof(*c) );

    rtmp = RTMP_Alloc();
    RTMP_Init(rtmp);
    RTMP_SetupURL(rtmp, stream_uri);
    RTMP_EnableWrite(rtmp); // To publish a stream, enable write support before connect
    RTMP_Connect(rtmp, NULL);
    RTMP_ConnectStream(rtmp, 0);

    if( !c )
    {
        free( c );
        return NULL;
    }

    return c;
}

int set_param( flv_hnd_t handle, x264_param_t *p_param )
{
    flv_hnd_t *p_flv = &handle;
    flv_buffer *c = p_flv->c;

    flv_put_byte( c, FLV_TAG_TYPE_META ); // Tag Type "script data"

    int start = c->d_cur;
    flv_put_be24( c, 0 ); // data length
    flv_put_be24( c, 0 ); // timestamp
    flv_put_be32( c, 0 ); // reserved

    flv_put_byte( c, AMF_DATA_TYPE_STRING );
    flv_put_amf_string( c, "onMetaData" );

    flv_put_byte( c, AMF_DATA_TYPE_MIXEDARRAY );
    flv_put_be32( c, 7 );

    flv_put_amf_string( c, "width" );
    flv_put_amf_double( c, p_param->i_width );

    flv_put_amf_string( c, "height" );
    flv_put_amf_double( c, p_param->i_height );

    flv_put_amf_string( c, "framerate" );

    if( !p_param->b_vfr_input )
        flv_put_amf_double( c, (double)p_param->i_fps_num / p_param->i_fps_den );
    else
    {
        p_flv->i_framerate_pos = c->d_cur + c->d_total + 1;
        flv_put_amf_double( c, 0 ); // written at end of encoding
    }

    flv_put_amf_string( c, "videocodecid" );
    flv_put_amf_double( c, FLV_CODECID_H264 );

    flv_put_amf_string( c, "duration" );
    p_flv->i_duration_pos = c->d_cur + c->d_total + 1;
    flv_put_amf_double( c, 0 ); // written at end of encoding

    flv_put_amf_string( c, "filesize" );
    p_flv->i_filesize_pos = c->d_cur + c->d_total + 1;
    flv_put_amf_double( c, 0 ); // written at end of encoding

    flv_put_amf_string( c, "videodatarate" );
    p_flv->i_bitrate_pos = c->d_cur + c->d_total + 1;
    flv_put_amf_double( c, 0 ); // written at end of encoding

    flv_put_amf_string( c, "" );
    flv_put_byte( c, AMF_END_OF_OBJECT );

    unsigned length = c->d_cur - start;
    flv_rewrite_amf_be24( c, length - 10, start );

    flv_put_be32( c, length + 1 ); // tag length

    p_flv->i_fps_num = p_param->i_fps_num;
    p_flv->i_fps_den = p_param->i_fps_den;
    p_flv->d_timebase = (double)p_param->i_timebase_num / p_param->i_timebase_den;
    p_flv->b_vfr_input = p_param->b_vfr_input;
    p_flv->i_delay_frames = p_param->i_bframe ? (p_param->i_bframe_pyramid ? 2 : 1) : 0;

    return 0;
}

int write_headers( flv_hnd_t handle, x264_nal_t *p_nal )
{
    flv_hnd_t *p_flv = &handle;
    flv_buffer *c = p_flv->c;

    int sps_size = p_nal[0].i_payload;
    int pps_size = p_nal[1].i_payload;
    int sei_size = p_nal[2].i_payload;

    // SEI
    /* It is within the spec to write this as-is but for
     * mplayer/ffmpeg playback this is deferred until before the first frame */

    p_flv->sei = malloc( sei_size );
    if( !p_flv->sei )
        return -1;
    p_flv->sei_len = sei_size;

    memcpy( p_flv->sei, p_nal[2].p_payload, sei_size );

    // SPS
    uint8_t *sps = p_nal[0].p_payload + 4;

    flv_put_byte( c, FLV_TAG_TYPE_VIDEO );
    flv_put_be24( c, 0 ); // rewrite later
    flv_put_be24( c, 0 ); // timestamp
    flv_put_byte( c, 0 ); // timestamp extended
    flv_put_be24( c, 0 ); // StreamID - Always 0
    p_flv->start = c->d_cur; // needed for overwriting length

    flv_put_byte( c, 7 | FLV_FRAME_KEY ); // Frametype and CodecID
    flv_put_byte( c, 0 ); // AVC sequence header
    flv_put_be24( c, 0 ); // composition time

    flv_put_byte( c, 1 );      // version
    flv_put_byte( c, sps[1] ); // profile
    flv_put_byte( c, sps[2] ); // profile
    flv_put_byte( c, sps[3] ); // level
    flv_put_byte( c, 0xff );   // 6 bits reserved (111111) + 2 bits nal size length - 1 (11)
    flv_put_byte( c, 0xe1 );   // 3 bits reserved (111) + 5 bits number of sps (00001)

    flv_put_be16( c, sps_size - 4 );
    flv_append_data( c, sps, sps_size - 4 );

    // PPS
    flv_put_byte( c, 1 ); // number of pps
    flv_put_be16( c, pps_size - 4 );
    flv_append_data( c, p_nal[1].p_payload + 4, pps_size - 4 );

    // rewrite data length info
    unsigned length = c->d_cur - p_flv->start;
    flv_rewrite_amf_be24( c, length, p_flv->start - 10 );
    flv_put_be32( c, length + 11 ); // Last tag size
    CHECK( flv_flush_data( c ) );

    return sei_size + sps_size + pps_size;
}

int write_frame( flv_hnd_t handle, uint8_t *p_nalu, int i_size, x264_picture_t *p_picture )
{
    flv_hnd_t *p_flv = &handle;
    flv_buffer *c = p_flv->c;

#define convert_timebase_ms( timestamp, timebase ) (int64_t)((timestamp) * (timebase) * 1000 + 0.5)

    if( !p_flv->i_framenum )
        p_flv->i_delay_time = p_picture->i_dts * -1;

    int64_t dts;
    int64_t cts;
    int64_t offset;

    if( p_flv->b_dts_compress )
    {
        if( p_flv->i_framenum == 1 )
            p_flv->i_init_delta = convert_timebase_ms( p_picture->i_dts + p_flv->i_delay_time, p_flv->d_timebase );
        dts = p_flv->i_framenum > p_flv->i_delay_frames
            ? convert_timebase_ms( p_picture->i_dts, p_flv->d_timebase )
            : p_flv->i_framenum * p_flv->i_init_delta / (p_flv->i_delay_frames + 1);
        cts = convert_timebase_ms( p_picture->i_pts, p_flv->d_timebase );
    }
    else
    {
        dts = convert_timebase_ms( p_picture->i_dts + p_flv->i_delay_time, p_flv->d_timebase );
        cts = convert_timebase_ms( p_picture->i_pts + p_flv->i_delay_time, p_flv->d_timebase );
    }
    offset = cts - dts;

    p_flv->i_prev_dts = dts;
    p_flv->i_prev_cts = cts;

    // A new frame - write packet header
    flv_put_byte( c, FLV_TAG_TYPE_VIDEO );
    flv_put_be24( c, 0 ); // calculated later
    flv_put_be24( c, dts );
    flv_put_byte( c, dts >> 24 );
    flv_put_be24( c, 0 );

    p_flv->start = c->d_cur;
    flv_put_byte( c, p_picture->b_keyframe ? FLV_FRAME_KEY : FLV_FRAME_INTER );
    flv_put_byte( c, 1 ); // AVC NALU
    flv_put_be24( c, offset );

    if( p_flv->sei )
    {
        flv_append_data( c, p_flv->sei, p_flv->sei_len );
        free( p_flv->sei );
        p_flv->sei = NULL;
    }
    flv_append_data( c, p_nalu, i_size );

    unsigned length = c->d_cur - p_flv->start;
    flv_rewrite_amf_be24( c, length, p_flv->start - 10 );
    flv_put_be32( c, 11 + length ); // Last tag size
    CHECK( flv_flush_data( c ) );

    p_flv->i_framenum++;

    return i_size;
}
