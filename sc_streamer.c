#include "sc_streamer.h"
#include <stdarg.h>
#include <sys/time.h>

extern tpl_hook_t tpl_hook;

sc_streamer sc_streamer_init_video(const char* stream_host, const char* room_name, sc_frame_rect capture_rect, sc_time start_time_stamp){
    x264_param_t param;
    
    char *stream_uri = (char *) malloc (100);
    sprintf(stream_uri, "rtmp://%s/screenshare/%s", stream_host, room_name);
    
    sc_streamer streamer = {.start_time_stamp = start_time_stamp,
        .stream_uri = stream_uri,
        .room_name = room_name,
        .capture_rect = capture_rect,
        .frames = 0,
        .rtmpt = 0};
    
    streamer.flv_out_handle = open_flv_buffer();
    streamer.rtmp = open_RTMP_stream( stream_uri );
    
    if(!RTMP_IsConnected(streamer.rtmp) || RTMP_IsTimedout(streamer.rtmp)) {
        streamer.rtmpt = 1;
        free(stream_uri);
        char *stream_uri = (char *) malloc (100);
        sprintf(stream_uri, "rtmpt://%s/screenshare/%s", stream_host, room_name);
        streamer.rtmp = open_RTMP_stream( stream_uri );
    }
    
    x264_param_default_preset(&param, "ultrafast", "zerolatency");
    x264_param_apply_profile(&param, "baseline");
    
    param.i_log_level  = X264_LOG_ERROR;
    //param.psz_dump_yuv = (char *)"/tmp/dump.y4m";
    
    param.b_vfr_input = 1;
    param.i_keyint_min = 1;
    param.i_keyint_max = 15;
    
    param.i_width = capture_rect.width;
    param.i_height = capture_rect.height;
    param.i_timebase_num   = 1.0;
    param.i_timebase_den   = SC_TimeBase;
    //Rate control/quality
    param.rc.i_rc_method = X264_RC_CRF;
    
    param.i_slice_max_size = 1024;
    param.rc.i_vbv_max_bitrate = 1000;
    param.rc.i_vbv_buffer_size = 2000;
    param.rc.f_rf_constant = 30;
    
    //    intra-refresh doesn't work w\ flash. Flash won't jump in w\out a keyframe
    param.b_repeat_headers = 1;
    param.b_annexb = 0;
    param.i_bframe = 0;
    
    streamer.encoder = x264_encoder_open(&param);
    
    set_param( streamer.flv_out_handle, &param );
    
    x264_nal_t *headers;
    int i_nal;
    
    x264_encoder_headers( streamer.encoder, &headers, &i_nal );
    write_headers( streamer.flv_out_handle, streamer.rtmp, headers );
    
    headers = NULL;
    free(headers);
    
    streamer.rtmp_setup = 1;
    streamer.reconnect_tries = 0;
    
    return streamer;
}

sc_streamer sc_streamer_init_cursor(const char* stream_host, const char* room_name, sc_time start_time_stamp){   
    char *so_name = (char *) malloc (100);
    sprintf(so_name, "SC.SS.%s.Cursor", room_name);
    
    char *stream_uri = (char *) malloc (100);
    sprintf(stream_uri, "rtmp://%s/screenshare/%s-cursor", stream_host, room_name);
    
    sc_streamer streamer = {.start_time_stamp = start_time_stamp,
        .stream_uri = stream_uri,
        .room_name = room_name,
        .so_name = so_name,
        .rtmpt = 0};
    
    streamer.flv_out_handle = open_flv_buffer();
    streamer.rtmp = open_RTMP_stream( stream_uri );
    
    if(!RTMP_IsConnected(streamer.rtmp) || RTMP_IsTimedout(streamer.rtmp)) {
        streamer.rtmpt = 1;
        free(stream_uri);
        char *stream_uri = (char *) malloc (100);
        sprintf(stream_uri, "rtmpt://%s/screenshare/%s-cursor", stream_host, room_name);
        streamer.rtmp = open_RTMP_stream( stream_uri );
    }
    
    setup_shared_object(streamer.so_name, streamer.rtmp);
    
    streamer.rtmp_setup = 1;
    streamer.so_version = 0;
    streamer.reconnect_tries = 0;
    
    return streamer;
}

void sc_streamer_send_frame(sc_streamer *streamer, sc_frame *frame, sc_time frame_time_stamp) {
    x264_picture_t pic_in, pic_out;
    x264_picture_alloc(&pic_in, X264_CSP_I420, streamer->capture_rect.width, streamer->capture_rect.height);
    
    uint8_t *YUV_frame = malloc(frame->size);
    memcpy(YUV_frame, frame->framePtr, frame->size);
    
    const size_t image_size = (streamer->capture_rect.width * streamer->capture_rect.height);
    
    free(pic_in.img.plane[0]);
    
    pic_in.img.plane[0] = YUV_frame;
    pic_in.img.plane[1] = YUV_frame + image_size;
    pic_in.img.plane[2] = YUV_frame + image_size + image_size / 4;
    
    pic_in.i_pts = floor((frame_time_stamp - streamer->start_time_stamp));
    
    x264_nal_t* nals;
    int i_nals;
    
    int frame_size = x264_encoder_encode(streamer->encoder, &nals, &i_nals, &pic_in, &pic_out);
    if(frame_size > 0) {
        write_frame( streamer->flv_out_handle, streamer->rtmp, nals[0].p_payload, frame_size, &pic_out );
        streamer->frames++;
    }
    
    YUV_frame = NULL;
    free(YUV_frame);
    nals = NULL;
    free(nals);
    
    x264_picture_clean(&pic_in);
}

void sc_streamer_send_mouse_data(sc_streamer *streamer, sc_mouse_coords *coords, sc_time coords_time_stamp) {
    //send_invoke( streamer.rtmp, coords.x, coords.y, coords_time_stamp, streamer.room_name );
    sc_mouse_coords *ret_coords = malloc(sizeof(sc_mouse_coords));
    memcpy(ret_coords, coords, sizeof(sc_mouse_coords));
    
    printf("sending SO: %i\n", streamer->so_version);
    update_x_y_and_timestamp(streamer->so_name, streamer->rtmp, coords->x, coords->y, coords_time_stamp, streamer->so_version);
}

void sc_streamer_stop_video(sc_streamer *streamer) {
    x264_encoder_close(streamer->encoder);
    close_RTMP_stream(*streamer->flv_out_handle, streamer->rtmp);
}

void sc_streamer_stop_cursor(sc_streamer *streamer) {
    close_RTMP_stream(*streamer->flv_out_handle, streamer->rtmp);
}

int main(int argc, char* argv[]) {  
    char c, *streamUri, *roomName, *inFile;
    sc_frame_rect rect;
    
    while ((c = getopt (argc, argv, "w:h:u:r:")) != -1) {
        switch (c) {
            case 'w':
                rect.width = (uint16_t) atoi(optarg);
                break;
            case 'h':
                rect.height = (uint16_t) atoi(optarg);
                break;
            case 'u':
                streamUri = optarg;
                break;
            case 'r':
                roomName = optarg;
                break;
                // case 'f':
                //     inFile = optarg;
                //     break;
                
        }
    }
    
    printf("Started streamer with width: %i, height: %i, URI: %s, roomName: %s\n", rect.width, rect.height, streamUri, roomName);
    
    // FILE *stream = freopen(inFile, "r", stdin);
    int fd = fileno(stdin);
    
    sc_streamer streamer;
    int have_inital_SO = 0;
    
    // main processing loop
    while(TRUE) {
        sc_bytestream_packet packet = sc_bytestream_get_event(fd);
        sc_mouse_coords coords;
        sc_frame frame;
        
        if(streamer.rtmp_setup == 1 && (!RTMP_IsConnected(streamer.rtmp) || RTMP_IsTimedout(streamer.rtmp))) {
            RTMP_Close(streamer.rtmp);
            RTMP_Free(streamer.rtmp);
            streamer.rtmp = open_RTMP_stream( streamer.stream_uri );
            
            streamer.reconnect_tries++;
            
            if(streamer.reconnect_tries >= SC_Reconnect_Attempts) {
                printf("Reconnected failed after %i tries", streamer.reconnect_tries);
                if(streamer.so_name != NULL) {
                    sc_streamer_stop_cursor(&streamer);
                } else {
                    sc_streamer_stop_video(&streamer);
                }
                exit(1);
            }
        }
        
        RTMPPacket rp;
        if(streamer.rtmp_setup == 1  && streamer.so_name != NULL && have_inital_SO != 1 && RTMP_ReadPacket(streamer.rtmp, &rp)) {
            if (RTMPPacket_IsReady(&rp) && rp.m_packetType == RTMP_PACKET_TYPE_SHARED_OBJECT)
            {
                AVal SO_name;
                AMF_DecodeString(rp.m_body, &SO_name);
                
                streamer.so_version = AMF_DecodeInt32(rp.m_body+2+SO_name.av_len);
                have_inital_SO = 1;
            }
        }
        
        switch(packet.header.type) {
            case STARTVIDEO:
                streamer = sc_streamer_init_video(streamUri, roomName, rect, packet.header.timestamp);
                break;
            case STOPVIDEO:
                sc_streamer_stop_video(&streamer);
                exit(1);
                break;
            case STARTCURSOR:
                streamer = sc_streamer_init_cursor(streamUri, roomName, packet.header.timestamp);
                break;
            case STOPCURSOR:
                sc_streamer_stop_cursor(&streamer);
                exit(1);
                break;
            case MOUSE:
                coords = parse_mouse_coords(packet);
                streamer.so_version++;
                sc_streamer_send_mouse_data(&streamer, &coords, packet.header.timestamp);
                break;
            case VIDEO:
                frame = parse_frame(packet);
                sc_streamer_send_frame(&streamer, &frame, packet.header.timestamp);
                break;
            case NO_DATA:
            default:
                break;    // maybe a wait is in order
        }
    }
    
    return 0;
}