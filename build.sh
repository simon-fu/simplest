gcc simplest_ffmpeg_muxer.cpp -o mux.bin -I /usr/local/include/ -L /usr/local/lib/ -lavcodec -lavformat  -lavutil
gcc simplest_ffmpeg_muxer_mov.cpp -o mov.bin -I /usr/local/include/ -L /usr/local/lib/ -lavcodec -lavformat -lavutil
gcc simplest_ffmpeg_muxer_simon.cpp -o simon.bin -I /usr/local/include/ -L /usr/local/lib/ -lavcodec -lavformat -lavutil
gcc muxing.c -o muxing.bin -I /usr/local/include/ -L /usr/local/lib/ -lavcodec -lavformat -lavutil -lswresample -lswscale


# ffmpeg library:
# avcodec.lib
# avformat.lib
# avutil.lib
# avdevice.lib
# avfilter.lib
# postproc.lib
# swresample.lib
# swscale.lib