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

	int audio_index = 0;
	int64_t cur_pts_v=0,cur_pts_a=0;
	FILE *fp_pts = fopen("audio_pts.txt", "w");

	//const char *in_filename_v = "cuc_ieschool.ts";//Input file URL
	//const char *in_filename_v = "cuc_ieschool.h264";
	const char *in_filename_v = "media.h264";
	//const char *in_filename_a = "cuc_ieschool.mp3";
	//const char *in_filename_a = "gowest.m4a";
	//const char *in_filename_a = "gowest.aac";
	//const char *in_filename_a = "huoyuanjia.mp3";
	const char *in_filename_a = "media.mp3";

	const char *out_filename = "media.mov";//Output file URL
	//const char *out_filename1 = "cuc_ieschool111.mov";//Output file URL
	av_register_all();
	//Input
	if ((ret = avformat_open_input(&ifmt_ctx_v, in_filename_v, 0, 0)) < 0) {
		printf( "Could not open input file.");
		// goto end;
		exit(-1);
	}
	if ((ret = avformat_find_stream_info(ifmt_ctx_v, 0)) < 0) {
		printf( "Failed to retrieve input stream information");
		// goto end;
		exit(-1);
	}

	if ((ret = avformat_open_input(&ifmt_ctx_a, in_filename_a, 0, 0)) < 0) {
		printf( "Could not open input file.");
		// goto end;
		exit(-1);
	}
	if ((ret = avformat_find_stream_info(ifmt_ctx_a, 0)) < 0) {
		printf( "Failed to retrieve input stream information");
		// goto end;
		exit(-1);
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
		// goto end;
		exit(-1);
	}
	
	AVCodecID codec_id = AV_CODEC_ID_H264;//视频 AV_CODEC_ID_MP3音频
	AVCodec *codec;
	AVStream *video_st;
	
	AVCodecContext *c;
	
	codec = avcodec_find_encoder(codec_id);
	video_st = avformat_new_stream(ofmt_ctx, codec);
	//video_st->time_base.num = 1;
	//video_st->time_base.den = 1200000;
	c = video_st->codec;
	avcodec_get_context_defaults3(c, codec);
	c->codec_id = codec_id;
	c->width = 240;
	c->height = 320;
	c->time_base.den = 15;
	c->time_base.num = 1;
	c->pix_fmt = PIX_FMT_YUV420P;
	c->bit_rate = 150000;
	/*c->sample_aspect_ratio.num = 1;
	c->sample_aspect_ratio.den = 1;*/
	videoindex_v = 0;
	videoindex_out = 0;
	
	video_st->codec->codec_tag = 0;
	if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;
		
	AVCodecID codec_id1 = AV_CODEC_ID_MP3;//视频 AV_CODEC_ID_MP3音频
	AVCodec *codec1;
	AVStream *video_st1;
	AVCodecContext *c1;
	/* find the video encoder */
	codec1 = avcodec_find_encoder(codec_id1);
	video_st1 = avformat_new_stream(ofmt_ctx, codec1);
	c1 = video_st1->codec;
	avcodec_get_context_defaults3(c1, codec1);
	c1->codec_id = codec_id1;
	c1->sample_rate = 16000;
	c1->channels = 1;
	c1->frame_size = 960;
	c1->bit_rate = 6000;
	c1->channel_layout = 4;
	c1->sample_fmt = AV_SAMPLE_FMT_S16P;
	c1->audio_service_type = AV_AUDIO_SERVICE_TYPE_MAIN;
	c1->block_align = 0;
	video_st1->codec->codec_tag = 0;
	if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		c1->flags |= CODEC_FLAG_GLOBAL_HEADER;

	audioindex_a = 0;
	audioindex_out = 1;
	ofmt = ofmt_ctx->oformat;

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
	//ofmt_ctx->streams[0]->codec->codec_id;

//	//Write file trailer
//	av_write_trailer(ofmt_ctx);
//
//#if USE_H264BSF
//	av_bitstream_filter_close(h264bsfc);
//#endif
//#if USE_AACBSF
//	av_bitstream_filter_close(aacbsfc);
//#endif
//
//	avformat_close_input(&ifmt_ctx_v);
//	avformat_close_input(&ifmt_ctx_a);
//	/* close output */
//	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
//		avio_close(ofmt_ctx->pb);
//	avformat_free_context(ofmt_ctx);
//	if (ret < 0 && ret != AVERROR_EOF) {
//		printf("Error occurred.\n");
//		return -1;
//	}


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
		av_init_packet(&pkt);
		//Get an AVPacket
		if (av_compare_ts(cur_pts_v, video_st->time_base, cur_pts_a, video_st1->time_base) <= 0/*av_compare_ts(cur_pts_v, ifmt_ctx_v->streams[videoindex_v]->time_base, cur_pts_a, ifmt_ctx_a->streams[audioindex_a]->time_base) <= 0*/){
			//视频
			ifmt_ctx=ifmt_ctx_v;
			stream_index=videoindex_out;

			if(av_read_frame(ifmt_ctx, &pkt) >= 0){
				do{
					//video_st = ifmt_ctx->streams[pkt.stream_index];
					out_stream = ofmt_ctx->streams[stream_index];

					if(pkt.stream_index==videoindex_v){
						//FIX：No PTS (Example: Raw H.264)
						//Simple Write PTS
						pkt.flags = 0;
						if(pkt.pts==AV_NOPTS_VALUE){
							//Write PTS
							AVRational time_base1 = video_st->time_base;
							//Duration between 2 frames (us)
							int64_t calc_duration = (double)AV_TIME_BASE / 15;
							//Parameters
							pkt.pts=(double)(frame_index*calc_duration)/(double)(av_q2d(time_base1)*AV_TIME_BASE);
							pkt.dts=pkt.pts;
							pkt.duration=(double)calc_duration/(double)(av_q2d(time_base1)*AV_TIME_BASE);
							frame_index++;
						}

						cur_pts_v=pkt.pts;
						break;
					}
				}while(av_read_frame(ifmt_ctx, &pkt) >= 0);
			}else{
				break;
			}
		}else{
			//音频
			ifmt_ctx=ifmt_ctx_a;
			stream_index=audioindex_out;
			if(av_read_frame(ifmt_ctx, &pkt) >= 0){
				do{
					in_stream  = ifmt_ctx->streams[pkt.stream_index];
					out_stream = ofmt_ctx->streams[stream_index];
					pkt.flags = 1;
					if(pkt.stream_index==audioindex_a){

						//FIX：No PTS
						//Simple Write PTS
						//if(pkt.pts==AV_NOPTS_VALUE){
							//Write PTS
							AVRational time_base1 = video_st1->time_base;
							//Duration between 2 frames (us)
							int64_t calc_duration = (double)AV_TIME_BASE*960 / 16000;
							//Parameters
							pkt.pts = (double)(audio_index*calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
							pkt.dts=pkt.pts;
							pkt.duration=(double)calc_duration/(double)(av_q2d(time_base1)*AV_TIME_BASE);
							audio_index++;
						//}
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

		if (stream_index == 0)
		{
			pkt.pts = av_rescale_q_rnd(pkt.pts, video_st->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.dts = av_rescale_q_rnd(pkt.dts, video_st->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.duration = av_rescale_q(pkt.duration, video_st->time_base, out_stream->time_base);
		}
		else
		{
			pkt.pts = av_rescale_q_rnd(pkt.pts, video_st1->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.dts = av_rescale_q_rnd(pkt.dts, video_st1->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.duration = av_rescale_q(pkt.duration, video_st1->time_base, out_stream->time_base);
		}
		//Convert PTS/DTS
		
		//pkt.pts = AV_NOPTS_VALUE;
		/*pkt.pts = AV_NOPTS_VALUE;
		pkt.dts = AV_NOPTS_VALUE;
		pkt.duration = 0;*/
		pkt.pos = -1;
		pkt.stream_index=stream_index;

		/*printf("Write 1 Packet. size:%5d\tpts:%lld\n",pkt.size,pkt.pts);*/
		//Write
		if (pkt.stream_index == videoindex_v)
		{
			//fprintf(fp_pts, "Write %d 视频 Packet. size:%5d\tpts:%lld\n", frame_index, pkt.size, pkt.pts);
			////printf("Write 视频 Packet. size:%5d\tpts:%lld\n", pkt.size, pkt.pts);
			if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
				printf("Error muxing packet\n");
				break;
			}
		}
		else
		{
			//printf("Write %d 音频 Packet. size:%5d\tpts:%lld\n", audio_index, pkt.size, pkt.pts);
			//fprintf(fp_pts, "Write %d audio Packet. size:%5d\tpts:%lld\n", audio_index, pkt.size, pkt.pts);
			if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
				printf("Error muxing packet\n");
				break;
			}
			
		}
		av_free_packet(&pkt);

	}
	//Write file trailer
	av_write_trailer(ofmt_ctx);
	fclose(fp_pts);

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


