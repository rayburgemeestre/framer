#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <chrono>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>

extern "C" {
  #include <libavutil/mathematics.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
}

class ffmpeg_streamer
{
private:
  std::string stream_mode_ = "";
  std::string url_ = "test.ts";
  uint32_t width_;
  uint32_t height_;

  // some properties
  AVOutputFormat *fmt;
  AVFormatContext *oc;
  AVStream *audio_st, *video_st;
  AVCodec *audio_codec, *video_codec;
  int ret;

  bool audio = false;

  AVFrame *frame;
  AVPicture src_picture, dst_picture;
  int frame_count = 0;

  std::chrono::high_resolution_clock::time_point current_time;

  size_t fps_;
  size_t bitrate_;

  bool streaming_ = false;

public:
  ffmpeg_streamer(std::string stream_mode, std::string url, size_t bitrate, size_t fps, uint32_t canvas_w, uint32_t canvas_h);
  void add_frame(std::vector<uint32_t> &pixels);
  void add_frame(const unsigned char *pixels);
  void finalize();

protected:
  AVStream *add_stream(AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id);
  void open_video(AVFormatContext *oc, AVCodec *codec, AVStream *st);
  void open_audio(AVFormatContext *oc, AVCodec *codec, AVStream *st);

  /**
    * Copy pixels to H264 frame pixels.
    *
    * @param pixels	raw pointer to all RGBA values from SFML image
    * @param c			struct that contains width and height for video
    * @param frame		video struct that contains pixels in YUV / YCrCb.
    */
  void transfer_pixels_avpicture(std::vector<uint32_t> &pixels, AVCodecContext * c, AVPicture *frame)
  {
    size_t index = 0;
    for(int y=0;y<c->height;y++) {
      for(int x=0;x<c->width;x++) {
        uint32_t &pixel(pixels[index++]);

//      uint8_t A = (pixel & 0x000000FF) >>  0;
//      uint8_t B = (pixel & 0x0000FF00) >>  8;
//      uint8_t G = (pixel & 0x00FF0000) >> 16;
//      uint8_t R = (pixel & 0xFF000000) >> 24;

        // SFML uses this
        uint8_t R = (pixel & 0x000000FF) >>  0;
        uint8_t G = (pixel & 0x0000FF00) >>  8;
        uint8_t B = (pixel & 0x00FF0000) >> 16;
        uint8_t A = (pixel & 0xFF000000) >> 24;

        // convert RGBA -> CMYK
        uint8_t Y  = static_cast<uint8_t>((0.257 * R) + (0.504 * G) + (0.098 * B) + 16);
        uint8_t Cb = static_cast<uint8_t>((-0.148 * R) - (0.291 * G) + (0.439 * B) + 128);
        uint8_t Cr = static_cast<uint8_t>((0.439 * R) - (0.368 * G) - (0.071 * B) + 128);

        frame->data[0][y * frame->linesize[0] + x] = Y;
        if ((y % 2) == 0 && (x % 2) == 0) {
          frame->data[1][(y/2) * frame->linesize[1] + (x/2)] = Cb;
          frame->data[2][(y/2) * frame->linesize[2] + (x/2)] = Cr;
        }
      }
    }
  }

  void fill_yuv_image(std::vector<uint32_t> &pixels, AVPicture *pict, int frame_index,
                             int width, int height)
  {
    AVCodecContext c;
    c.width = width;
    c.height = height;
    c.pix_fmt = AV_PIX_FMT_YUV420P;
    transfer_pixels_avpicture(pixels, &c, pict);
  }

  void write_video_frame(std::vector<uint32_t> &pixels, AVFormatContext *oc, AVStream *st);
  void close_video(AVFormatContext *oc, AVStream *st);
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

/*
 * Copyright (c) 2003 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * libavformat API example.
 *
 * Output a media file in any supported libavformat format.
 * The default codecs are used.
 * @example doc/examples/muxing.c
 */



//#include "streamer_output/ffmpeg_stream_hls.h"
//#include "benchmark.h"

/* 5 seconds stream duration */
//#define STREAM_DURATION   200.0
extern int STREAM_FRAME_RATE;// = 2; /* 25 images/s */
//#define STREAM_NB_FRAMES  ((int)(STREAM_DURATION * STREAM_FRAME_RATE))
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

static int sws_flags = SWS_BICUBIC;

/**************************************************************/
/* audio output */

static float t, tincr, tincr2;
static int16_t *samples;
static int audio_input_frame_size;

extern "C" {
  #include "libavutil/opt.h"
}


// copied from libavformat/hlsenc.c (not available in a header it seems)
enum HLSFlags {
  HLS_SINGLE_FILE = (1 << 0),
  HLS_DELETE_SEGMENTS = (1 << 1),
  HLS_ROUND_DURATIONS = (1 << 2),
  HLS_DISCONT_START = (1 << 3),
  HLS_OMIT_ENDLIST = (1 << 4),
};



ffmpeg_streamer::ffmpeg_streamer(std::string stream_mode, std::string url, size_t bitrate, size_t fps, uint32_t canvas_w, uint32_t canvas_h)
  : stream_mode_(std::move(stream_mode)), url_(std::move(url)), width_(canvas_w), height_(canvas_h), fps_(fps), bitrate_(bitrate)
{
  std::cout << "ffmpeg_streamer using bitrate " << bitrate_ << " and fps of " << fps_ << std::endl;

//  timer = std::move(t);
  current_time = std::chrono::high_resolution_clock::now();



  /* Initialize libavcodec, and register all codecs and formats. */
  av_register_all();

    avformat_network_init(); //?? here??

  /* allocate the output media context */


  if (stream_mode_ == "hls") {
    avformat_alloc_output_context2(&oc, nullptr, "hls", url_.c_str());
    streaming_ = true;
  }
  else if (stream_mode_ == "rtmp") {
    avformat_alloc_output_context2(&oc, nullptr, "flv", url_.c_str());
    streaming_ = true;
  }
  else {
    avformat_alloc_output_context2(&oc, nullptr, stream_mode_.c_str(), url_.c_str());
  }





  if (!oc) {
    printf("Could not deduce output format from file extension: using mp4.\n");
    avformat_alloc_output_context2(&oc, nullptr, "mp4", url_.c_str());
  }
  if (!oc) {
    printf("ERRO\n");
    return;
  }
  fmt = oc->oformat;

  /* Add the audio and video streams using the default format codecs
      * and initialize the codecs. */
  video_st = nullptr;
  audio_st = nullptr;


  if (fmt->video_codec != AV_CODEC_ID_NONE) {
    video_st = add_stream(oc, &video_codec, fmt->video_codec);
  }
  if (audio) if (fmt->audio_codec != AV_CODEC_ID_NONE) {
      std::cout << "RBU: adding audio stream" << std::endl;
      audio_st = add_stream(oc, &audio_codec, fmt->audio_codec);
    }

  /* Now that all the parameters are set, we can open the audio and
      * video codecs and allocate the necessary encode buffers. */
  if (video_st)
    open_video(oc, video_codec, video_st);
  if (audio) if (audio_st) {
    std::cout << "RBU: opening audio! " << std::endl;
      open_audio(oc, audio_codec, audio_st);
    }




    av_dump_format(oc, 0, url_.c_str(), 1);

	// useless though
    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oc->pb, url_.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open '%s': %d\n", url_.c_str(),
                    (ret));
            return;
        }
    }


    if (stream_mode_ == "hls") {
      // a bit ugly but let's just postfix the .m3u8 file..
      std::string segment_filename_pattern = url_ + "_%d.ts";
      av_opt_set(oc->priv_data, "hls_segment_filename", segment_filename_pattern.c_str(), 0);
      av_opt_set_int(oc->priv_data, "hls_list_size", 10, 0);
      av_opt_set_int(oc->priv_data, "hls_time", 1, 0);
      av_opt_set_int(oc->priv_data, "hls_flags", HLSFlags::HLS_OMIT_ENDLIST | HLSFlags::HLS_DELETE_SEGMENTS, 0);
    }


  /* Write the stream header, if any. */
  ret = avformat_write_header(oc, nullptr);
  if (ret < 0) {
    fprintf(stderr, "Error occurred when opening output file: %d\n",
            (ret));
    return;
  }

//  timer->start();

//  // fix a few warnings..
  frame->format = STREAM_PIX_FMT;
  frame->width = width_;
  frame->height = height_;

  auto noww = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> idle = noww - current_time;
  frame->pts = idle.count() * (this->stream_mode_ == "hls" ? 100.0 : 1.0);
  //if (!streaming_) frame->pts = (1 / 25. /* fps */ ) * 4410. * frame_count++; //(550 * frame_count++); // FPS
  if (!streaming_) frame->pts = 1; // start with one.
}

/* Add an output stream. */
AVStream *ffmpeg_streamer::add_stream(AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id)
{
  AVCodecContext *c;
  AVStream *st;

  /* find the encoder */
  *codec = avcodec_find_encoder(codec_id);
  if (!(*codec)) {
    fprintf(stderr, "Could not find encoder for '%s'\n",
            avcodec_get_name(codec_id));
    exit(1);
  }

  st = avformat_new_stream(oc, *codec);
  if (!st) {
    fprintf(stderr, "Could not allocate stream\n");
    exit(1);
  }
  st->id = oc->nb_streams-1;
  c = st->codec;

  c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  switch ((*codec)->type) {
    case AVMEDIA_TYPE_AUDIO:
      st->id = 1;
      // requires ./configure --enable-libfdk_aac which apparently is not supported on Ubuntu by default (18.04)
      c->sample_fmt  = AV_SAMPLE_FMT_S16P;
//      c->sample_fmt  = AV_SAMPLE_FMT_DBL;
      c->sample_fmt = AV_SAMPLE_FMT_FLTP;
    //c->bit_rate    = 64000;
      c->bit_rate    = 24000;
      c->sample_rate = 44100;
      c->channels    = 2;
      // added by trigen: correct?
      //c->time_base.den = STREAM_FRAME_RATE;
      //c->time_base.num = 1;
      // end
      break;

    case AVMEDIA_TYPE_VIDEO:
      c->codec_id = codec_id;

      c->bit_rate = bitrate_;
      /* Resolution must be a multiple of two. */
      c->width    = width_;
      c->height   = height_;
      /* timebase: This is the fundamental unit of time (in seconds) in terms
          * of which frame timestamps are represented. For fixed-fps content,
          * timebase should be 1/framerate and timestamp increments should be
          * identical to 1. */
      c->time_base.den = fps_; // STREAM_FRAME_RATE;
      c->time_base.num = 1;

      c->gop_size      = 12; /* emit one intra frame every twelve frames at most */
      c->pix_fmt       = STREAM_PIX_FMT;
      if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B frames */
        c->max_b_frames = 2;
      }
      if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
        /* Needed to avoid using macroblocks in which some coeffs overflow.
            * This does not happen with normal video, it just happens here as
            * the motion of the chroma plane does not match the luma plane. */
        c->mb_decision = 2;
      }


      break;

    default:
      break;
  }

  /* Some formats want stream headers to be separate. */
  if (oc->oformat->flags & AVFMT_GLOBALHEADER)
    c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    return st;
}

/**************************************************************/
/* audio output */

void ffmpeg_streamer::open_audio(AVFormatContext *oc, AVCodec *codec, AVStream *st)
{
  AVCodecContext *c;
  int ret;

  c = st->codec;

  /* open it */
  ret = avcodec_open2(c, codec, nullptr);
  if (ret < 0) {
    fprintf(stderr, "Could not open audio codec: %d\n", ret);
    exit(1);
  }

  /* init signal generator */
  t     = 0;
  tincr = 2 * M_PI * 110.0 / c->sample_rate;
  /* increment frequency by 110 Hz per second */
  tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

  if (c->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE)
    audio_input_frame_size = 10000;
  else
    audio_input_frame_size = c->frame_size;
  samples = (int16_t *)av_malloc(audio_input_frame_size *
                                 av_get_bytes_per_sample(c->sample_fmt) *
                                 c->channels);
  if (!samples) {
    fprintf(stderr, "Could not allocate audio samples buffer\n");
    exit(1);
  }
}

/* Prepare a 16 bit dummy audio frame of 'frame_size' samples and
	* 'nb_channels' channels. */
static void get_audio_frame(int16_t *samples, int frame_size, int nb_channels)
{
  std::cout << "generating audio frame" << std::endl;
  int j, i, v;
  int16_t *q;

  q = samples;
  static float tbak = t;
  static float tincrbak = tincr;
  for (j = 0; j < frame_size; j++) {
    v = (int)(sin(t) * 10000);
    for (i = 0; i < nb_channels; i++)
      *q++ = v;
    t     += tincr;
    tincr += tincr2;

    if (t > 500000) {
      t = tbak;
      tincr = tincrbak;
    }
  }
}

static void write_audio_frame(AVFormatContext *oc, AVStream *st)
{
  std::cout << "write audio frame! <" << std::endl;
  AVCodecContext *c;
  AVPacket pkt = { 0 }; // data and size must be 0;
  AVFrame *frame = av_frame_alloc();

  int got_packet, ret;

  av_init_packet(&pkt);
  c = st->codec;

  get_audio_frame(samples, audio_input_frame_size, c->channels);
  frame->nb_samples = audio_input_frame_size;
  avcodec_fill_audio_frame(frame, c->channels, c->sample_fmt,
                           (uint8_t *)samples,
                           audio_input_frame_size *
                           av_get_bytes_per_sample(c->sample_fmt) *
                           c->channels, 1);

  ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
  if (ret < 0) {
    fprintf(stderr, "Error encoding audio frame: %d\n", (ret));
    exit(1);
  }

  if (!got_packet)
    return;

  pkt.stream_index = st->index;

  /* Write the compressed frame to the media file. */
  ret = av_interleaved_write_frame(oc, &pkt);
  if (ret != 0) {
    fprintf(stderr, "Error while writing audio frame: %d\n",
            ret);
    exit(1);
  }
  av_frame_free(&frame);
}

static void close_audio(AVFormatContext *oc, AVStream *st)
{
  avcodec_close(st->codec);

  av_free(samples);
}

/**************************************************************/
/* video output */

void ffmpeg_streamer::open_video(AVFormatContext *oc, AVCodec *codec, AVStream *st)
{
  int ret;
  AVCodecContext *c = st->codec;

  /* open the codec */
  ret = avcodec_open2(c, codec, nullptr);
  if (ret < 0) {
    fprintf(stderr, "Could not open video codec: %d\n", (ret));
    exit(1);
  }

  /* allocate and init a re-usable frame */
  frame = av_frame_alloc();
  if (!frame) {
    fprintf(stderr, "Could not allocate video frame\n");
    exit(1);
  }

  /* Allocate the encoded raw picture. */
  ret = avpicture_alloc(&dst_picture, c->pix_fmt, c->width, c->height);
  if (ret < 0) {
    fprintf(stderr, "Could not allocate picture: %d\n", (ret));
    exit(1);
  }

  /* If the output format is not YUV420P, then a temporary YUV420P
      * picture is needed too. It is then converted to the required
      * output format. */
  if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
    ret = avpicture_alloc(&src_picture, AV_PIX_FMT_YUV420P, c->width, c->height);
    if (ret < 0) {
      fprintf(stderr, "Could not allocate temporary picture: %d\n",
              (ret));
      exit(1);
    }
  }

  /* copy data and linesize picture pointers to frame */
  *((AVPicture *)frame) = dst_picture;
}

void ffmpeg_streamer::write_video_frame(std::vector<uint32_t> &pixels, AVFormatContext *oc, AVStream *st)
{
  int ret;
  static struct SwsContext *sws_ctx;
  AVCodecContext *c = st->codec;

  if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
    /* as we only generate a YUV420P picture, we must convert it
        * to the codec pixel format if needed */
    if (!sws_ctx) {
      sws_ctx = sws_getContext(c->width, c->height, AV_PIX_FMT_YUV420P,
                               c->width, c->height, c->pix_fmt,
                               sws_flags, nullptr, nullptr, nullptr);
      if (!sws_ctx) {
        fprintf(stderr,
                "Could not initialize the conversion context\n");
        exit(1);
      }
    }
    fill_yuv_image(pixels, &src_picture, frame_count, c->width, c->height);
    sws_scale(sws_ctx,
              (const uint8_t * const *)src_picture.data, src_picture.linesize,
              0, c->height, dst_picture.data, dst_picture.linesize);
  } else {
    fill_yuv_image(pixels, &dst_picture, frame_count, c->width, c->height);
  }

  if (oc->oformat->flags & AVFMT_RAWPICTURE) {
    //unused block
    /* Raw video case - directly store the picture in the packet */
    AVPacket pkt;
    av_init_packet(&pkt);

    pkt.flags        |= AV_PKT_FLAG_KEY;
    pkt.stream_index  = st->index;
    pkt.data          = dst_picture.data[0];
    pkt.size          = sizeof(AVPicture);

    ret = av_interleaved_write_frame(oc, &pkt);
  } else {
    AVPacket pkt = { 0 };
    av_init_packet(&pkt);

    /* encode the image */
#define USE_DEPRECATED_API
#ifdef USE_DEPRECATED_API
    int got_packet = 0;
    /*
    ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
      fprintf(stderr, "Error encoding audio frame: %d\n", (ret));
      exit(1);
    }
    */
    ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
      fprintf(stderr, "Error encoding video frame: %d\n", (ret));
      exit(1);
    }
    if (got_packet) {
        // this is for when we're processing files I suppose; fwrite(pkt.data, 1, pkt.size, f);
     //   av_packet_unref(&pkt);
    }
#else
    ret = avcodec_send_frame(c, frame);
    if (ret < 0) {
      fprintf(stderr, "Error encoding video frame: %d\n", (ret));
      exit(1);
    }
    ret = avcodec_receive_packet(c, &pkt);
#endif

//    if (ret == AVERROR(EAGAIN)) { fprintf(stderr, "error one"); }
//    if (ret == AVERROR_EOF) { fprintf(stderr, "error two"); }
//    if (ret == AVERROR(EINVAL)) { fprintf(stderr, "error three"); }

  if (stream_mode_ == "hls") {
    // For some reason avcodec_receive_packet always fails for HLS, surpressing seems to work..
    ret = 0;
  }
    
	if (ret < 0) {
      fprintf(stderr, "Error encoding video frame (2): %d\n", (ret));
      exit(1);
    }
    /* If size is zero, it means the image was buffered. */

    if (!ret && /* got_packet && */ pkt.size) {
      pkt.stream_index = st->index;

      /* Write the compressed frame to the media file. */
      ret = av_interleaved_write_frame(oc, &pkt);
    } else {
      ret = 0;
    }
    // The following line may not be needed, added just in case.
    av_free(pkt.data);
  }
  if (ret != 0) {
    fprintf(stderr, "Error while writing video frame: %d\n", (ret));
    exit(1);
  }
  frame_count++;
}

void ffmpeg_streamer::close_video(AVFormatContext *oc, AVStream *st)
{
  //avcodec_close(st->codec);
  av_free(src_picture.data[0]);
  av_free(dst_picture.data[0]);
  av_free(frame);
}

/**************************************************************/
/* media file output */
#include <iostream>
#include <string>
#include <stdio.h>
#include <time.h>

void ffmpeg_streamer::add_frame(std::vector<uint32_t> &pixels) {
 // static auto previous = frame->pts;
//  std::cout << "RBU: add_frame! - " << frame->pts  << " from: " << previous << " diff: " << (frame->pts - previous) << std::endl;
//  previous = frame->pts;
  //sfml_generate_frame();

  auto noww = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> idle = noww - current_time;
  int64_t endTime = static_cast<int64_t>(idle.count() * (this->stream_mode_ == "hls" ? 100.0 : 1.0));

  // Make sure to write enough audio frames until the audio timer lines up
  if (audio)
    std::cout << "audio = true" << std::endl;
    while (true) {
      double audio_pts = (audio_st) ? (double)audio_st->pts.val * audio_st->time_base.num / audio_st->time_base.den : 0.0;
      double video_pts = (video_st) ? (double)video_st->pts.val * video_st->time_base.num / video_st->time_base.den : 0.0;
      std::cout << " audio : " << audio_pts << " , video: " << video_pts << std::endl;
      std::cout << " audio : " << audio_st->pts.val << " , video: " << video_st->pts.val << std::endl;

      if (audio_pts >= video_pts)
        break;

      std::cout << "+A";
      write_audio_frame(oc, audio_st);

    }



  // Write video frame
  write_video_frame(pixels, oc, video_st);

  std::cout << "+V" << std::endl;

//  bool encode_video = true;
//  bool encode_audio = audio;
//
//  while (encode_video || encode_audio) {
//    /* select the stream to encode */
//    if (encode_video &&
//        (!encode_audio || av_compare_ts(video_st->next_pts, video_st->enc->time_base,
//                                        audio_st->next_pts, audio_st->enc->time_base) <= 0)) {
//      encode_video = !write_video_frame(pixels, oc, video_st);
//    } else {
//      encode_audio = !write_audio_frame(oc, audio_st);
//    }
//  }




  if (!streaming_) {
      endTime = av_rescale_q(1, video_st->codec->time_base, video_st->time_base);
      frame->pts += endTime;
  } else {
      frame->pts = endTime;
  }
}

void ffmpeg_streamer::add_frame(const unsigned char *rawpixels)
{
  // GET RID OF THIS STATIC, ITS JUST FOR TESTING NOW
  static std::vector<uint32_t> pixels;
  size_t index = 0;

  pixels.reserve(width_ * height_);
  // TODO: don't loop over pixels just make sure the formats are the same
  for (unsigned int y = 0; y < height_; y++) {
    for (unsigned int x = 0; x < width_; x++) {
      pixels[index++] = *((uint32_t *)rawpixels);
      rawpixels += sizeof(uint32_t) / sizeof(unsigned char);
    }
  }
  add_frame(pixels);
}


void ffmpeg_streamer::finalize() {
  /* Write the trailer, if any. The trailer must be written before you
   * close the CodecContexts open when you wrote the header; otherwise
   * av_write_trailer() may try to use memory that was freed on
   * av_codec_close(). */
  av_write_trailer(oc);

  /* Close each codec. */
  if (video_st)
    close_video(oc, video_st);
  if (audio_st)
    close_audio(oc, audio_st);

  if (!(fmt->flags & AVFMT_NOFILE))
    /* Close the output file. */
    avio_close(oc->pb);

  /* free the stream */
  avformat_free_context(oc);
}

#pragma GCC diagnostic pop

