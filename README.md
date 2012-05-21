ScreenStreamer
======

Small process for streaming bytestreamed data via an RTMP stream.

Debugging
======

Some tips for debugging issues.

### Step 1

If you uncomment the `param.psz_dump_yuv` parameter, the raw video stream will be dumped to file so that
you can check the output for corruption or errors.

The resulting file will be playable using YAY (http://freecode.com/projects/yay). You may also need
to install the sdl libs (`brew install sdl`) beforehand. After install is complete, run:

`yay -s [screen width]x[screen height] /tmp/dump.y4m`

### Step 2

Once the app opens, use key 'p' to play, and 'r' to rewind and view. See the YAY README for more details.

Because we're using a bytestream go-between, we have the ability to record a sequence of events and then play
them back at will if needed. In order to do this, have a tpl-encoded bytestream recording available. Simply
uncomment the `case 'f'` statement and add a 'f:' option to the getopt function. Uncomment the `FILE *stream`
line and make sure that `fileno()` is getting the file handle for `stream` instead of `stdin`. Then simply
run the `screen_streamer` binary from the command line with all the appropriate options. For example:

`./screen_streamer -urtmp://example.com:1935/screenshare/roomId -rroomId -w1440 -h900 -frunnableStream.tpl`

This will play back the stream file and actually stream the recorded events to the fms server and/or dump to
the dump.ym4 file if the dump param is uncommented (see above).

License
======

Both [libx264][1] and librtmp are both licensed under [GPL2][2].

[1]: http://www.videolan.org/developers/x264.html
[2]: http://www.gnu.org/licenses/gpl-2.0.html