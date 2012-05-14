//
//  sc_streamer.c
//  Screen Share
//
//  Created by Gabe Bell on 4/6/12.
//  Copyright (c) 2012 Salescrunch, Inc. All rights reserved.
//

#include "sc_streamer.h"



sc_streamer sc_streamer_init(const char* stream_uri, const char* room_name, sc_frame_rect capture_rect, sc_time start_time_stamp){
    x264_param_t param;

    sc_streamer sc_streamer = {.start_time_stamp = start_time_stamp,
        .stream_uri = stream_uri,
        .room_name = room_name,
        .capture_rect = capture_rect};

    RTMP *rtmp = open_RTMP_stream( stream_uri, &sc_streamer.flv_out_handle );
    sc_streamer.rtmp = rtmp;

    x264_param_default_preset(&param, "ultrafast", "zerolatency");
    x264_param_apply_profile(&param, "baseline");

    param.i_log_level  = X264_LOG_INFO;
    //param.psz_dump_yuv = (char *)"/tmp/dump.y4m";

    param.b_vfr_input = 1;

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

    sc_streamer.encoder = x264_encoder_open(&param);

    set_param( sc_streamer.flv_out_handle, &param );

    x264_nal_t *headers;
    int i_nal;

    x264_encoder_headers( sc_streamer.encoder, &headers, &i_nal );
    write_headers( sc_streamer.flv_out_handle, headers );

    headers = NULL;
    free(headers);

    return sc_streamer;
}

void sc_streamer_send_frame(sc_streamer streamer, uint8_t* YUV_frame, sc_time frame_time_stamp) {
    x264_picture_t pic_in, pic_out;
    x264_picture_alloc(&pic_in, X264_CSP_I420, streamer.capture_rect.width, streamer.capture_rect.height);

    const size_t image_size = (streamer.capture_rect.width * streamer.capture_rect.height);

    free(pic_in.img.plane[0]);

    pic_in.img.plane[0] = YUV_frame;
    pic_in.img.plane[1] = YUV_frame + image_size;
    pic_in.img.plane[2] = YUV_frame + image_size + image_size / 4;

    pic_in.i_pts = floor((frame_time_stamp - streamer.start_time_stamp)/(1.0/SC_TimeBase));

    x264_nal_t* nals;
    int i_nals;

    int frame_size = x264_encoder_encode(streamer.encoder, &nals, &i_nals, &pic_in, &pic_out);

    if(frame_size > 0) {
       write_frame( streamer.flv_out_handle, nals[0].p_payload, frame_size, &pic_out );
    }

    streamer.frames++;
    YUV_frame = NULL;
    free(YUV_frame);
    nals = NULL;
    free(nals);
    x264_picture_clean(&pic_in);
}

// TODO: get room name
void sc_streamer_send_mouse_data(sc_streamer streamer, sc_mouse_coords coords, sc_time coords_time_stamp) {
    send_invoke( streamer.rtmp, coords.x, coords.y, coords_time_stamp, streamer.room_name );
}

void sc_streamer_stop(sc_streamer streamer) {
    x264_encoder_close(streamer.encoder);
    close_RTMP_stream(streamer.flv_out_handle, streamer.rtmp);
}


int main() { return 0; }