/**
 * 最简单的基于FFmpeg的视音频复用器
 * Simplest FFmpeg Muxer
 *
 * 雷霄骅 Lei Xiaohua
 * leixiaohua1020@126.com
 * 中国传媒大学/数字电视技术
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 *
 * 本程序可以将视频码流和音频码流打包到一种封装格式中。
 * 程序中将AAC编码的音频码流和H.264编码的视频码流打包成
 * MPEG2TS封装格式的文件。
 * 需要注意的是本程序并不改变视音频的编码格式。
 *
 * This software mux a video bitstream and a audio bitstream 
 * together into a file.
 * In this example, it mux a H.264 bitstream (in MPEG2TS) and 
 * a AAC bitstream file together into MP4 format file.
 *
 */

#include <stdio.h>

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavformat/avformat.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavformat/avformat.h>
#ifdef __cplusplus
};
#endif
#endif

/*
FIX: H.264 in some container format (FLV, MP4, MKV etc.) need 
"h264_mp4toannexb" bitstream filter (BSF)
  *Add SPS,PPS in front of IDR frame
  *Add start code ("0,0,0,1") in front of NALU
H.264 in some container (MPEG2TS) don't need this BSF.
*/
//'1': Use H.264 Bitstream Filter 
#define USE_H264BSF 0

/*
FIX:AAC in some container format (FLV, MP4, MKV etc.) need 
"aac_adtstoasc" bitstream filter (BSF)
*/
//'1': Use AAC Bitstream Filter 
#define USE_AACBSF 0


static
void dump_codec_ctx(const char * pre, AVCodecContext * c)
{
	printf("%s c->codec_id=%d\n", pre, c->codec_id);
	printf("%s c->width=%d\n", pre, c->width);
	printf("%s c->height=%d\n", pre, c->height);
	printf("%s c->time_base.den=%d\n", pre, c->time_base.den);
	printf("%s c->time_base.num=%d\n", pre, c->time_base.num);
	printf("%s c->pix_fmt=%d\n", pre, c->pix_fmt);
	printf("%s c->bit_rate=%d\n", pre, c->bit_rate);

	printf("%s c->sample_rate=%d\n", pre, c->sample_rate);
	printf("%s c->channels=%d\n", pre, c->channels);
	printf("%s c->frame_size=%d\n", pre, c->frame_size);
	printf("%s c->channel_layout=%llu\n", pre, c->channel_layout);
	printf("%s c->sample_fmt=%d\n", pre, c->sample_fmt);
	printf("%s c->audio_service_type=%d\n", pre, c->audio_service_type);
	printf("%s c->block_align=%d\n", pre, c->block_align);
	printf("%s c->codec_tag=%d\n", pre, c->codec_tag);

}

static void open_video(AVCodecContext *c, AVCodec *codec,  AVDictionary *opt_arg)
{
    int ret;
    // AVCodecContext *c = ost->st->codec;
    AVDictionary *opt = NULL;
    av_dict_copy(&opt, opt_arg, 0);
    /* open the codec */
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        fprintf(stderr, "Could not open video codec: %s\n", av_err2str(ret));
        exit(1);
    }
}

static void open_audio(AVCodecContext *c, AVCodec *codec, AVDictionary *opt_arg)
{
    // AVCodecContext *c;
    int nb_samples;
    int ret;
    AVDictionary *opt = NULL;
    // c = ost->st->codec;
    /* open it */
    av_dict_copy(&opt, opt_arg, 0);
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        fprintf(stderr, "Could not open audio codec: %s\n", av_err2str(ret));
        exit(1);
    }
}


static 
AVStream * config_codec_ctx_video( AVFormatContext * ofmt_ctx){
	AVCodecID codec_id = AV_CODEC_ID_H264;
	AVCodec *codec = avcodec_find_encoder(codec_id);

		AVStream *st = avformat_new_stream(ofmt_ctx, codec);
		if (!st) {
			printf( "Failed allocating output video stream\n");
			exit(-1);
		}

	AVCodecContext * c = st->codec;


	// avcodec_get_context_defaults3(c, codec);
	
	c->codec_id = codec_id;
	c->width = 240;
	c->height = 320;
	// c->time_base.den = 15;
	c->time_base.den = 30; // by simon
	c->time_base.num = 1;
	c->pix_fmt = PIX_FMT_YUV420P;
	//c->bit_rate = 150000;
	c->bit_rate = 0;  // by simon
	c->qmin = 2;	// by simon
	c->qmax = 31;	// by simon
	/*c->sample_aspect_ratio.num = 1;
	c->sample_aspect_ratio.den = 1;*/
	
	c->codec_tag = 0;
	if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;

	ofmt_ctx->oformat->video_codec = codec_id;
	st->time_base = (AVRational){ c->time_base.num, c->time_base.den };

	open_video(c, codec, NULL);
	return st;
}

static 
AVStream * config_codec_ctx_audio(AVFormatContext * ofmt_ctx){
	AVCodecID codec_id1 = AV_CODEC_ID_MP3;
	AVCodec *codec1 = avcodec_find_encoder(codec_id1);

		AVStream *st = avformat_new_stream(ofmt_ctx, codec1);
		if (!st) {
			printf( "Failed allocating output audio stream\n");
			exit(-1);
		}

	AVCodecContext * c1 = st->codec;


	// avcodec_get_context_defaults3(c1, codec1);
	c1->codec_id = codec_id1;
	c1->sample_rate = 16000;
	c1->channels = 1;
	//c1->frame_size = 960;
	c1->frame_size = 576; // by simon
	//c1->bit_rate = 6000;
	c1->bit_rate = 24000; // by simon
	c1->channel_layout = 4;
	c1->sample_fmt = AV_SAMPLE_FMT_S16P;
	c1->audio_service_type = AV_AUDIO_SERVICE_TYPE_MAIN;
	c1->block_align = 0;
	// add by simon
	c1->time_base.den = 0; 
	c1->time_base.num = 1;

	c1->codec_tag = 0;
	if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		c1->flags |= CODEC_FLAG_GLOBAL_HEADER;

	ofmt_ctx->oformat->audio_codec = codec_id1;
	st->time_base = (AVRational){ 1, c1->sample_rate };
	open_audio(c1, codec1, NULL);
	return st;
}

int main(int argc, char* argv[])
{
	AVOutputFormat *ofmt = NULL;
	//Input AVFormatContext and Output AVFormatContext
	AVFormatContext *ifmt_ctx_v = NULL, *ifmt_ctx_a = NULL,*ofmt_ctx = NULL;
	AVPacket pkt;
	int ret, i;
	int videoindex_v=-1,videoindex_out=-1;
	int audioindex_a=-1,audioindex_out=-1;
	int frame_index=0;
	int64_t cur_pts_v=0,cur_pts_a=0;

	//const char *in_filename_v = "cuc_ieschool.ts";//Input file URL
	const char *in_filename_v = "media.h264";
	//const char *in_filename_a = "cuc_ieschool.mp3";
	//const char *in_filename_a = "gowest.m4a";
	//const char *in_filename_a = "gowest.aac";
	const char *in_filename_a = "media.mp3";

	const char *out_filename = "simon.mov";//Output file URL
	av_register_all();
	//Input
	if ((ret = avformat_open_input(&ifmt_ctx_v, in_filename_v, 0, 0)) < 0) {
		printf( "Could not open input file.");
		goto end;
	}
	if ((ret = avformat_find_stream_info(ifmt_ctx_v, 0)) < 0) {
		printf( "Failed to retrieve input stream information");
		goto end;
	}

	if ((ret = avformat_open_input(&ifmt_ctx_a, in_filename_a, 0, 0)) < 0) {
		printf( "Could not open input file.");
		goto end;
	}
	if ((ret = avformat_find_stream_info(ifmt_ctx_a, 0)) < 0) {
		printf( "Failed to retrieve input stream information");
		goto end;
	}


	printf("===========Input Information==========\n");
	av_dump_format(ifmt_ctx_v, 0, in_filename_v, 0);
	av_dump_format(ifmt_ctx_a, 0, in_filename_a, 0);
	printf("======================================\n");
	//Output
	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
	if (!ofmt_ctx) {
		printf( "Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	ofmt = ofmt_ctx->oformat;

	for (i = 0; i < ifmt_ctx_v->nb_streams; i++) {

		//Create output AVStream according to input AVStream
		if(ifmt_ctx_v->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){

		AVStream *in_stream = ifmt_ctx_v->streams[i];
		printf("input video codec %p\n", in_stream->codec->codec);

		// AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
		// videoindex_v=i;
		// if (!out_stream) {
		// 	printf( "Failed allocating output stream\n");
		// 	ret = AVERROR_UNKNOWN;
		// 	goto end;
		// }
		// videoindex_out=out_stream->index;

		// //Copy the settings of AVCodecContext
		// if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
		// 	printf( "Failed to copy context from input to output stream codec context\n");
		// 	goto end;
		// }
		// out_stream->codec->codec_tag = 0;
		// if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		// 	out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

		// out_stream->id = ofmt_ctx->nb_streams-1;
		AVStream *out_stream = config_codec_ctx_video(ofmt_ctx);

		videoindex_v=i;
		videoindex_out=out_stream->index;

		dump_codec_ctx("video", out_stream->codec);
		break;

		
		}
	}

	for (i = 0; i < ifmt_ctx_a->nb_streams; i++) {
		//Create output AVStream according to input AVStream
		if(ifmt_ctx_a->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){

			AVStream *in_stream = ifmt_ctx_a->streams[i];
			printf("input audio codec %p\n", in_stream->codec->codec);

			// AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
			// audioindex_a=i;
			// if (!out_stream) {
			// 	printf( "Failed allocating output stream\n");
			// 	ret = AVERROR_UNKNOWN;
			// 	goto end;
			// }
			// audioindex_out=out_stream->index;

			// //Copy the settings of AVCodecContext
			// if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
			// 	printf( "Failed to copy context from input to output stream codec context\n");
			// 	goto end;
			// }
			// out_stream->codec->codec_tag = 0;
			// if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			// 	out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

			// out_stream->id = ofmt_ctx->nb_streams-1;
			AVStream *out_stream = config_codec_ctx_audio(ofmt_ctx);

			audioindex_a=i;
			audioindex_out=out_stream->index;


			dump_codec_ctx("audio", out_stream->codec);

			break;
		}
	}

	printf("==========Output Information==========\n");
	av_dump_format(ofmt_ctx, 0, out_filename, 1);
	printf("======================================\n");
	//Open output file
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		if (avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE) < 0) {
			printf( "Could not open output file '%s'", out_filename);
			goto end;
		}
	}
	//Write file header
	if (avformat_write_header(ofmt_ctx, NULL) < 0) {
		printf( "Error occurred when opening output file\n");
		goto end;
	}


	//FIX
#if USE_H264BSF
	AVBitStreamFilterContext* h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb"); 
#endif
#if USE_AACBSF
	AVBitStreamFilterContext* aacbsfc =  av_bitstream_filter_init("aac_adtstoasc"); 
#endif

	while (1) {
		AVFormatContext *ifmt_ctx;
		int stream_index=0;
		AVStream *in_stream, *out_stream;

		//Get an AVPacket
		if(av_compare_ts(cur_pts_v,ifmt_ctx_v->streams[videoindex_v]->time_base,cur_pts_a,ifmt_ctx_a->streams[audioindex_a]->time_base) <= 0){
			ifmt_ctx=ifmt_ctx_v;
			stream_index=videoindex_out;

			if(av_read_frame(ifmt_ctx, &pkt) >= 0){
				do{
					in_stream  = ifmt_ctx->streams[pkt.stream_index];
					out_stream = ofmt_ctx->streams[stream_index];

					if(pkt.stream_index==videoindex_v){
						//FIX：No PTS (Example: Raw H.264)
						//Simple Write PTS
						if(pkt.pts==AV_NOPTS_VALUE){
							// printf("video input => no pts\n");
							//Write PTS
							AVRational time_base1=in_stream->time_base;
							//Duration between 2 frames (us)
							int64_t calc_duration=(double)AV_TIME_BASE/av_q2d(in_stream->r_frame_rate);
							//Parameters
							pkt.pts=(double)(frame_index*calc_duration)/(double)(av_q2d(time_base1)*AV_TIME_BASE);
							pkt.dts=pkt.pts;
							pkt.duration=(double)calc_duration/(double)(av_q2d(time_base1)*AV_TIME_BASE);
							frame_index++;
						}else{
							// printf("video input => has pts\n");
						}
						printf("video: pts=%lld\n", pkt.pts);
						cur_pts_v=pkt.pts;
						break;
					}
				}while(av_read_frame(ifmt_ctx, &pkt) >= 0);
			}else{
				break;
			}
		}else{
			ifmt_ctx=ifmt_ctx_a;
			stream_index=audioindex_out;
			if(av_read_frame(ifmt_ctx, &pkt) >= 0){
				do{
					in_stream  = ifmt_ctx->streams[pkt.stream_index];
					out_stream = ofmt_ctx->streams[stream_index];

					if(pkt.stream_index==audioindex_a){

						//FIX：No PTS
						//Simple Write PTS
						if(pkt.pts==AV_NOPTS_VALUE){
							// printf("audio input => no pts");
							//Write PTS
							AVRational time_base1=in_stream->time_base;
							//Duration between 2 frames (us)
							int64_t calc_duration=(double)AV_TIME_BASE/av_q2d(in_stream->r_frame_rate);
							//Parameters
							pkt.pts=(double)(frame_index*calc_duration)/(double)(av_q2d(time_base1)*AV_TIME_BASE);
							pkt.dts=pkt.pts;
							pkt.duration=(double)calc_duration/(double)(av_q2d(time_base1)*AV_TIME_BASE);
							frame_index++;
						}else{
							// printf("audio input => has pts\n");
						}
						printf("audio: pts=%lld\n", pkt.pts);

						cur_pts_a=pkt.pts;

						break;
					}
				}while(av_read_frame(ifmt_ctx, &pkt) >= 0);
			}else{
				break;
			}

		}

		//FIX:Bitstream Filter
#if USE_H264BSF
		av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif
#if USE_AACBSF
		av_bitstream_filter_filter(aacbsfc, out_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif


		//Convert PTS/DTS
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;
		pkt.stream_index=stream_index;

		//printf("Write 1 Packet. size:%5d\tpts:%lld\n",pkt.size,pkt.pts);
		//Write
		if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
			printf( "Error muxing packet\n");
			break;
		}
		av_free_packet(&pkt);

	}
	//Write file trailer
	av_write_trailer(ofmt_ctx);

#if USE_H264BSF
	av_bitstream_filter_close(h264bsfc);
#endif
#if USE_AACBSF
	av_bitstream_filter_close(aacbsfc);
#endif

end:
	avformat_close_input(&ifmt_ctx_v);
	avformat_close_input(&ifmt_ctx_a);
	/* close output */
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);
	if (ret < 0 && ret != AVERROR_EOF) {
		printf( "Error occurred.\n");
		return -1;
	}
	return 0;
}


