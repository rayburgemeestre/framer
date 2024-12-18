// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

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

#pragma once

#include <functional>
#include <iostream>
#include <thread>
#include <utility>

template <typename F>
class scope_guard {
  F f;
  bool active = true;

public:
  scope_guard(F &&f) : f(std::forward<F>(f)) {}
  ~scope_guard() {
    if (active) f();
  }
  void dismiss() { active = false; }
  scope_guard(scope_guard &&) = default;
  scope_guard &operator=(scope_guard &&) = default;
  scope_guard(const scope_guard &) = delete;
  scope_guard &operator=(const scope_guard &) = delete;
};

namespace sg {
template <typename F>
scope_guard<F> make_scope_guard(F &&f) {
  return scope_guard<F>(std::forward<F>(f));
}
}  // namespace sg

// As shown in below comment, this code was based on the muxing.c example from the FFmpeg source code.
// Then kept up to date with the latest FFmpeg API changes since version +/- 3.0.

/**
 * @file
 * libavformat API example.
 *
 * Output a media file in any supported libavformat format. The default
 * codecs are used.
 * @example muxing.c
 */

#include <chrono>
#include <vector>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavformat/avformat.h>
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

// copied from libavformat/hlsenc.c (not available in a header it seems)
enum HLSFlags {
  HLS_SINGLE_FILE = (1 << 0),
  HLS_DELETE_SEGMENTS = (1 << 1),
  HLS_ROUND_DURATIONS = (1 << 2),
  HLS_DISCONT_START = (1 << 3),
  HLS_OMIT_ENDLIST = (1 << 4),
};

#define SCALE_FLAGS SWS_BICUBIC

// Below was required after updating to Ubuntu 18.04.
#ifdef __cplusplus
static const std::string av_make_error_string(int errnum) {
  char errbuf[AV_ERROR_MAX_STRING_SIZE];
  av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
  return (std::string)errbuf;
}

#undef av_err2str
#define av_err2str(errnum) av_make_error_string(errnum).c_str()

static const std::string av_ts_make_string(int64_t ts) {
  char buf[AV_TS_MAX_STRING_SIZE];
  if (ts == AV_NOPTS_VALUE)
    snprintf(buf, AV_TS_MAX_STRING_SIZE, "NOPTS");
  else
    snprintf(buf, AV_TS_MAX_STRING_SIZE, "%" PRId64, ts);
  return (std::string)buf;
}
#undef av_ts2str
#define av_ts2str(ts) av_ts_make_string(ts).c_str()

static const std::string av_ts_make_time_string(int64_t ts, AVRational *tb) {
  char buf[AV_TS_MAX_STRING_SIZE];
  if (ts == AV_NOPTS_VALUE)
    snprintf(buf, AV_TS_MAX_STRING_SIZE, "NOPTS");
  else
    snprintf(buf, AV_TS_MAX_STRING_SIZE, "%.6g", av_q2d(*tb) * ts);
  return buf;
}
#undef av_ts2timestr
#define av_ts2timestr(ts, tb) av_ts_make_time_string(ts, tb).c_str()
#endif  // __cplusplus

class frame_streamer;
static frame_streamer *global_this = nullptr;

static void av_log_callback(void *ptr, int level, const char *fmt, va_list vl);

class frame_streamer {
private:
  // int STREAM_DURATION   = 10 /* seconds */;
  // int STREAM_FRAME_RATE = 25 /* 25 images/s */;
  AVPixelFormat STREAM_PIX_FMT = AV_PIX_FMT_YUV420P /* default pix_fmt */;

  // a wrapper around a single output AVStream
  typedef struct OutputStream {
    AVStream *st;
    AVCodecContext *enc;

    /* pts of the next frame that will be generated */
    int64_t next_pts;
    int samples_count;

    AVFrame *frame;
    AVFrame *tmp_frame;

    float t, tincr, tincr2;

    struct SwsContext *sws_ctx;
    struct SwrContext *swr_ctx;
  } OutputStream;

  OutputStream video_st = {nullptr}, audio_st = {nullptr};
  const AVOutputFormat *fmt;
  AVFormatContext *oc;
  const AVCodec *audio_codec, *video_codec;
  int ret;
  int have_video = 0, have_audio = 0;
  int encode_video = 0, encode_audio = 0;
  AVDictionary *opt = nullptr;

public:
  enum class stream_mode { FILE, RTMP, HLS };
  enum class color_mode { BGRA, RGBA };

  // TODO: why does this need to be public
  std::function<void(int level, const std::string &line)> log_callback = nullptr;
  int log_callback_level = 0;
  std::string log_callback_buffer;

private:
  bool initialized_;
  stream_mode mode_;
  color_mode cmode_;
  std::string filename_;
  size_t bitrate_;
  size_t fps_;
  size_t width_;
  size_t height_;
  std::chrono::high_resolution_clock::time_point current_time_;
  std::chrono::steady_clock::time_point start_time_;
  int num_threads_ = -1;  // sentinel value for do not override default
  std::function<void(float seconds, int fps, int num_channels, int16_t *channels)> audio_callback_ = nullptr;
  std::function<void(std::vector<unsigned int> &pixels, int width, int height)> video_callback_ = nullptr;
  int64_t audio_pts = 0;
  int64_t video_pts = 0;
  bool streams_configured_ = false;
  bool running_ = true;

public:
  frame_streamer(std::string filename,
                 size_t bitrate,
                 int fps,
                 int width,
                 int height,
                 stream_mode mode = stream_mode::FILE,
                 color_mode cmode = color_mode::RGBA)
      : initialized_(true),
        mode_(mode),
        cmode_(cmode),
        filename_(std::move(filename)),
        bitrate_(bitrate),
        fps_(fps),
        width_(width),
        height_(height),
        current_time_(std::chrono::high_resolution_clock::now()),
        start_time_(std::chrono::steady_clock::now()) {}

  /**
   * Constructor that does not yet take all parameters, the idea is to use initialize() later.
   */
  frame_streamer(std::string filename, stream_mode mode = stream_mode::FILE, color_mode cmode = color_mode::RGBA)
      : initialized_(false),
        mode_(mode),
        cmode_(cmode),
        filename_(std::move(filename)),
        bitrate_(0),
        fps_(0),
        width_(0),
        height_(0),
        current_time_(std::chrono::high_resolution_clock::now()),
        start_time_(std::chrono::steady_clock::now()) {}

  void initialize(size_t bitrate, int width, int height, int fps) {
    bitrate_ = bitrate;
    width_ = width;
    height_ = height;
    fps_ = fps;
    audio_pts = 0;
    video_pts = 0;
    initialized_ = true;
    _configure_streams();
  }

  void set_log_callback(std::function<void(int level, const std::string &line)> log_callback) {
    this->log_callback = log_callback;
  }

  void set_audio_callback(
      std::function<void(float seconds, int fps, int num_channels, int16_t *channels)> audio_callback) {
    this->audio_callback_ = audio_callback;
    _configure_streams();
  }

  void set_video_callback(
      std::function<void(std::vector<unsigned int> &pixels, int width, int height)> video_callback) {
    this->video_callback_ = video_callback;
    _configure_streams();
  }

  bool _is_audio_enabled() { return audio_callback_ != nullptr; }
  bool _is_video_callback_enabled() { return video_callback_ != nullptr; }

  void set_num_threads(int num_threads) { num_threads_ = num_threads; }

  bool is_streaming() { return mode_ != stream_mode::FILE; }

private:
  int _configure_streams() {
    if (streams_configured_) {
      return 0;
    }
    /* Initialize libavcodec, and register all codecs and formats. */
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
    av_register_all();
#endif

    // for (int i = 2; i+1 < argc; i+=2) {
    //     if (!strcmp(argv[i], "-flags") || !strcmp(argv[i], "-fflags"))
    //         av_dict_set(&opt, argv[i]+1, argv[i+1], 0);
    // }

    /* allocate the output media context */
    switch (mode_) {
      case stream_mode::FILE:
        avformat_alloc_output_context2(&oc, nullptr, nullptr, filename_.c_str());
        break;
      case stream_mode::RTMP:
        avformat_alloc_output_context2(&oc, nullptr, "flv", filename_.c_str());
        break;
      case stream_mode::HLS:
        avformat_alloc_output_context2(&oc, nullptr, "hls", filename_.c_str());
        // a bit ugly but let's just postfix the .m3u8 file..
        std::string segment_filename_pattern = filename_ + "_%d.ts";
        av_opt_set(oc->priv_data, "hls_segment_filename", segment_filename_pattern.c_str(), 0);
        av_opt_set_int(oc->priv_data, "hls_list_size", 10, 0);
        av_opt_set_int(oc->priv_data, "hls_time", 1, 0);
        av_opt_set_int(oc->priv_data, "hls_flags", HLSFlags::HLS_OMIT_ENDLIST | HLSFlags::HLS_DELETE_SEGMENTS, 0);
        break;
    }

    if (!oc) {
      printf("Could not deduce output format from file extension: using MPEG.\n");
      avformat_alloc_output_context2(&oc, nullptr, "mpeg", filename_.c_str());
    }
    if (!oc) return 1;

    fmt = oc->oformat;

    /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
    if (fmt->video_codec != AV_CODEC_ID_NONE) {
      add_stream(&video_st, oc, &video_codec, fmt->video_codec);
      have_video = 1;
      encode_video = 1;
    }
    if (_is_audio_enabled()) {
      if (fmt->audio_codec != AV_CODEC_ID_NONE) {
        add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec);
        have_audio = 1;
        encode_audio = 1;
      }
    }

    if (log_callback != nullptr) {
      global_this = this;
      av_log_set_callback(av_log_callback);
    }

    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */
    if (have_video) open_video(oc, video_codec, &video_st, opt);

    if (_is_audio_enabled()) {
      if (have_audio) {
        open_audio(oc, audio_codec, &audio_st, opt);
      }
    }

    av_dump_format(oc, 0, filename_.c_str(), 1);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
      ret = avio_open(&oc->pb, filename_.c_str(), AVIO_FLAG_WRITE);
      if (ret < 0) {
        fprintf(stderr, "Could not open '%s': %s\n", filename_.c_str(), av_err2str(ret));
        throw std::runtime_error("Continuing will fail in mux.c, because avformat_write_header is not optional.");
      }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, &opt);
    if (ret < 0) {
      fprintf(stderr, "Error occurred when opening output file: %s\n", av_err2str(ret));
      return 1;
    }
    streams_configured_ = true;
    return 0;
  }

  std::vector<uint32_t> *pixels_ = nullptr;  // temporary pointer

public:
  void add_frame(std::vector<uint32_t> &pixels) {
    _configure_streams();
    while (encode_video || encode_audio) {
      if (encode_video &&
          (!encode_audio ||
           av_compare_ts(video_st.next_pts, video_st.enc->time_base, audio_st.next_pts, audio_st.enc->time_base) <=
               0)) {
        pixels_ = &pixels;
        encode_video = !write_video_frame(oc, &video_st);
        break;
      } else if (encode_audio) {
        encode_audio = !write_audio_frame(oc, &audio_st);
      }
    }
  }

  void add_frame(const uint8_t *rawpixels, int width, int height) {
    std::vector<uint32_t> pixels;
    size_t index = 0;
    pixels.reserve(width * height);
    for (unsigned int y = 0; y < (unsigned int)height; y++) {
      for (unsigned int x = 0; x < (unsigned int)width; x++) {
        pixels[index++] = *((uint32_t *)rawpixels);
        rawpixels += sizeof(uint32_t) / sizeof(unsigned char);
      }
    }
    add_frame(pixels);
  }

  void run_loop() {
    if (!_is_video_callback_enabled()) {
      throw std::runtime_error("video callback not enabled");
    }
    _configure_streams();
    auto stream_start = std::chrono::steady_clock::now();
    const double frame_duration = 1.0 / fps_;  // Duration of one frame in seconds
    int frames = 0;
    while (running_) {
      std::vector<uint32_t> pixels(width_ * height_, 0x00000000);
      while (encode_video || encode_audio) {
        if (encode_video &&
            (!encode_audio ||
             av_compare_ts(video_st.next_pts, video_st.enc->time_base, audio_st.next_pts, audio_st.enc->time_base) <=
                 0)) {
          double target_time = frames * frame_duration;
          // Get actual elapsed time
          auto now = std::chrono::steady_clock::now();
          double elapsed = std::chrono::duration<double>(now - stream_start).count();
          if (elapsed > target_time + frame_duration) {
            // We're running too slow - skip this frame
            frames++;
            continue;
          }
          if (elapsed < target_time) {
            // We're running too fast - sleep until target time
            std::this_thread::sleep_for(std::chrono::duration<double>(target_time - elapsed));
          }

          video_callback_(pixels, width_, height_);
          pixels_ = &pixels;  // TODO: pass it around?
          encode_video = !write_video_frame(oc, &video_st);
          frames++;
          break;
        } else if (encode_audio) {
          encode_audio = !write_audio_frame(oc, &audio_st);
        }
      }
    }
  }

  void record() {
    _configure_streams();
    while (running_) {
      write_audio_frame(oc, &audio_st);
    }
  }
  void stop() { running_ = false; }

  void finalize() {
    if (!initialized_) return;

    /* Write the trailer, if any. The trailer must be written before you
     * close the CodecContexts open when you wrote the header; otherwise
     * av_write_trailer() may try to use memory that was freed on
     * av_codec_close(). */
    av_write_trailer(oc);

    /* Close each codec. */
    if (have_video) close_stream(oc, &video_st);
    if (have_audio) close_stream(oc, &audio_st);

    if (!(fmt->flags & AVFMT_NOFILE)) {
      // This ensures all buffers are flushed to disk
      avio_flush(oc->pb);
      /* Close the output file. */
      avio_closep(&oc->pb);
    }

    /* free the stream */
    avformat_free_context(oc);
    initialized_ = false;
  }

private:
  void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt) {
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;
    char buf[512] = {0x00};
    snprintf(buf,
             512,
             "pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
             av_ts2str(pkt->pts),
             av_ts2timestr(pkt->pts, time_base),
             av_ts2str(pkt->dts),
             av_ts2timestr(pkt->dts, time_base),
             av_ts2str(pkt->duration),
             av_ts2timestr(pkt->duration, time_base),
             pkt->stream_index);
    buf[512 - 1] = 0x00;
    if (!global_this) {
      printf("%s", buf);
      return;
    }
    global_this->log_callback_buffer.append(buf);
    if (global_this->log_callback_buffer[global_this->log_callback_buffer.size() - 1] == '\n') {
      global_this->log_callback(global_this->log_callback_level, global_this->log_callback_buffer);
      global_this->log_callback_buffer = "";
    }
  }

  int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt) {
    /* rescale output packet timestamp values from codec to stream timebase */
    av_packet_rescale_ts(pkt, *time_base, st->time_base);
    pkt->stream_index = st->index;

    /* Write the compressed frame to the media file. */
    log_packet(fmt_ctx, pkt);
    return av_interleaved_write_frame(fmt_ctx, pkt);
  }

  /* Add an output stream. */
  void add_stream(OutputStream *ost, AVFormatContext *oc, const AVCodec **codec, enum AVCodecID codec_id) {
    AVCodecContext *c;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
      fprintf(stderr, "Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
      exit(1);
    }

    ost->st = avformat_new_stream(oc, nullptr);
    if (!ost->st) {
      fprintf(stderr, "Could not allocate stream\n");
      exit(1);
    }
    ost->st->id = oc->nb_streams - 1;
    c = avcodec_alloc_context3(*codec);
    if (!c) {
      fprintf(stderr, "Could not alloc an encoding context\n");
      exit(1);
    }
    ost->enc = c;

    switch ((*codec)->type) {
      case AVMEDIA_TYPE_AUDIO: {
#if LIBAVCODEC_VERSION_MAJOR >= 61
        // This is for our custom ffmpeg version build
        const enum AVSampleFormat *p = NULL;
        avcodec_get_supported_config(NULL, *codec, AV_CODEC_CONFIG_SAMPLE_FORMAT, 0, (const void **)&p, NULL);
        c->sample_fmt = p ? p[0] : AV_SAMPLE_FMT_FLTP;
        c->bit_rate = 64000;
        c->sample_rate = 44100;
#else
        c->sample_fmt = (*codec)->sample_fmts ? (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
        int i = 0;
#endif

        c->bit_rate = 64000;
        c->sample_rate = 44100;
#if LIBAVCODEC_VERSION_MAJOR >= 61
        int *supported_samplerates = nullptr;
        avcodec_get_supported_config(
            NULL, *codec, AV_CODEC_CONFIG_SAMPLE_RATE, 0, (const void **)&supported_samplerates, NULL);

        // This is for our custom ffmpeg version build
        if (supported_samplerates) {
          c->sample_rate = supported_samplerates[0];
          // Look for 44.1kHz specifically
          for (int i = 0; supported_samplerates[i]; i++) {
            if (supported_samplerates[i] == 44100) {
              c->sample_rate = 44100;
              break;
            }
          }
          // Free the allocated memory
          // below crashes
          // av_free(supported_samplerates);
        }
#else
        if ((*codec)->supported_samplerates) {
          c->sample_rate = (*codec)->supported_samplerates[0];
          for (i = 0; (*codec)->supported_samplerates[i]; i++) {
            if ((*codec)->supported_samplerates[i] == 44100) {
              c->sample_rate = 44100;
              break;
            }
          }
        }
#endif

        // Initialize default stereo layout
#if LIBAVCODEC_VERSION_MAJOR >= 61
        if (av_channel_layout_from_mask(&c->ch_layout, AV_CH_LAYOUT_STEREO) < 0) {
          fprintf(stderr, "Could not set stereo layout\n");
          exit(1);
        }
#else
        av_channel_layout_default(&c->ch_layout, 2);  // 2 for stereo
#endif

#if LIBAVCODEC_VERSION_MAJOR >= 61
        // Get supported channel layouts
        const AVChannelLayout *supported_layouts = nullptr;
        avcodec_get_supported_config(
            NULL, *codec, AV_CODEC_CONFIG_CHANNEL_LAYOUT, 0, (const void **)&supported_layouts, NULL);

        if (supported_layouts) {
          // Create our desired stereo layout
          AVChannelLayout stereo_layout = {.order = AV_CHANNEL_ORDER_NATIVE};
          av_channel_layout_from_mask(&stereo_layout, AV_CH_LAYOUT_STEREO);

          // Look for stereo support in the supported layouts
          for (int i = 0; supported_layouts[i].nb_channels; i++) {
            if (av_channel_layout_compare(&supported_layouts[i], &stereo_layout) == 0) {
              av_channel_layout_copy(&c->ch_layout, &stereo_layout);
              break;
            }

            // Store first layout as fallback
            if (i == 0) {
              av_channel_layout_copy(&c->ch_layout, &supported_layouts[0]);
            }
          }

          // Clean up our temporary layout
          // TODO: is this correct, it doesn't crash as with the sample rates
          av_channel_layout_uninit(&stereo_layout);
          av_free((void *)supported_layouts);
        }
#else
        // Check codec's supported channel layouts
        if ((*codec)->ch_layouts) {
          AVChannelLayout stereo_layout = {.order = AV_CHANNEL_ORDER_NATIVE};
          av_channel_layout_from_mask(&stereo_layout, AV_CH_LAYOUT_STEREO);

          for (i = 0; (*codec)->ch_layouts[i].nb_channels; i++) {
            if (av_channel_layout_compare(&(*codec)->ch_layouts[i], &stereo_layout) == 0) {
              av_channel_layout_copy(&c->ch_layout, &stereo_layout);
              break;
            }

            if (i == 0) {
              av_channel_layout_copy(&c->ch_layout, &(*codec)->ch_layouts[0]);
            }
          }
        }
#endif

        ost->st->time_base = AVRational{1, audio_st.enc->sample_rate};  // Usually 1/44100
        c->time_base = ost->st->time_base;

        if (num_threads_ != -1) {
          c->thread_count = num_threads_;
        }
        break;
      }
      case AVMEDIA_TYPE_VIDEO:
        c->codec_id = codec_id;
        c->bit_rate = bitrate_;

        // cranking up resolution can bump the profile level to:
        // [libx264 @ 0x7fee50035ae0] profile High, level 6.1
        // results in browser: creating sourceBuffer(video/mp4;codecs=avc1.64003d)
        // and fails with:
        // [error] > error while trying to add sourceBuffer:MediaSource.addSourceBuffer: Can't play type
        // see this URL for details:
        // https://privacycheck.sec.lrz.de/active/fp_cpt/fp_can_play_type.html#video/mp4;%20codecs=%22avc1.64003d%22

        // anyway, this seems to still work in the browser:
        // [libx264 @ 0x7fe13422e1c0] profile High, level 5.2
        // so let's force this profile level for now
        // results in: creating sourceBuffer(video/mp4;codecs=avc1.640034)
        // and doesn't result in an error in the browser
        // av_opt_set(oc->priv_data, "profile", "high", 0);
        // av_opt_set(oc->priv_data, "profile", "baseline", AV_OPT_SEARCH_CHILDREN);

        // more info about profiles and levels here:
        //  https://sonnati.wordpress.com/2008/10/25/a-primer-to-h-264-levels-and-profiles/
        c->profile = FF_PROFILE_H264_BASELINE;
        c->profile = FF_PROFILE_H264_MAIN;
        c->profile = FF_PROFILE_H264_HIGH;
        // c->profile = FF_PROFILE_H264_HIGH_444_PREDICTIVE;

        // laptop supports streaming up to profile level 5.2. in the browser
        // we should make this and the profile configurable.
        c->level = 52;

        if (num_threads_ != -1) {
          c->thread_count = num_threads_;
        }

        /* Resolution must be a multiple of two. */
        c->width = width_;
        c->height = height_;
        /* timebase: This is the fundamental unit of time (in seconds) in terms
         * of which frame timestamps are represented. For fixed-fps content,
         * timebase should be 1/framerate and timestamp increments should be
         * identical to 1. */
        if (mode_ == stream_mode::HLS) {
          ost->st->time_base = AVRational{1, 90000};  // Standard 90kHz clock for MPEG/HLS
          c->time_base = ost->st->time_base;
        } else {
          ost->st->time_base = AVRational{1, (int)fps_};
          c->time_base = ost->st->time_base;
        }

        c->gop_size = 12; /* emit one intra frame every twelve frames at most */
        c->pix_fmt = STREAM_PIX_FMT;
        if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
          /* just for testing, we also add B-frames */
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
    if (oc->oformat->flags & AVFMT_GLOBALHEADER) c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  /**************************************************************/
  /* audio output */

  AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                             const AVChannelLayout *ch_layout,
                             int sample_rate,
                             int nb_samples) {
    AVFrame *frame = av_frame_alloc();
    int ret;

    if (!frame) {
      fprintf(stderr, "Error allocating an audio frame\n");
      exit(1);
    }

    frame->format = sample_fmt;
    av_channel_layout_copy(&frame->ch_layout, ch_layout);  // Copy from codec context

    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples) {
      ret = av_frame_get_buffer(frame, 0);
      if (ret < 0) {
        fprintf(stderr, "Error allocating an audio buffer\n");
        exit(1);
      }
    }

    return frame;
  }

  void open_audio(AVFormatContext *oc, const AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg) {
    AVCodecContext *c;
    int nb_samples;
    int ret;
    AVDictionary *opt = nullptr;

    c = ost->enc;

    /* open it */
    av_dict_copy(&opt, opt_arg, 0);
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
      fprintf(stderr, "Could not open audio codec: %s\n", av_err2str(ret));
      exit(1);
    }

    /* init signal generator */
    ost->t = 0;
    ost->tincr = static_cast<float>(2 * M_PI * 110.0 / c->sample_rate);
    /* increment frequency by 110 Hz per second */
    ost->tincr2 = static_cast<float>(2 * M_PI * 110.0 / c->sample_rate / c->sample_rate);

    if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
      nb_samples = 10000;
    else
      nb_samples = c->frame_size;

    ost->frame = alloc_audio_frame(c->sample_fmt, &c->ch_layout, c->sample_rate, nb_samples);

    AVChannelLayout tmp_layout;
    av_channel_layout_copy(&tmp_layout, &c->ch_layout);

    ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, &tmp_layout, c->sample_rate, nb_samples);

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
      fprintf(stderr, "Could not copy the stream parameters\n");
      exit(1);
    }

    /* create resampler context */
    ost->swr_ctx = swr_alloc();
    if (!ost->swr_ctx) {
      fprintf(stderr, "Could not allocate resampler context\n");
      exit(1);
    }

    /* set options */
    av_opt_set_chlayout(ost->swr_ctx, "in_chlayout", &c->ch_layout, 0);
    av_opt_set_chlayout(ost->swr_ctx, "out_chlayout", &c->ch_layout, 0);
    av_opt_set_int(ost->swr_ctx, "in_sample_rate", c->sample_rate, 0);
    av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int(ost->swr_ctx, "out_sample_rate", c->sample_rate, 0);
    av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt", c->sample_fmt, 0);

    /* initialize the resampling context */
    if ((ret = swr_init(ost->swr_ctx)) < 0) {
      fprintf(stderr, "Failed to initialize the resampling context\n");
      exit(1);
    }
  }

  AVFrame *get_audio_frame(OutputStream *ost) {
    AVFrame *frame = ost->tmp_frame;
    int16_t *q = (int16_t *)frame->data[0];  // I don't know why but passing this pointer to the callback doesn't work
    int16_t *tmp = (int16_t *)av_malloc(ost->enc->ch_layout.nb_channels * sizeof(int16_t));
    memset(tmp, 0, ost->enc->ch_layout.nb_channels * sizeof(int16_t));

    for (int j = 0; j < frame->nb_samples; j++) {
      float t = float(ost->next_pts + j) / ost->enc->sample_rate;
      float seconds = t;

      if (audio_callback_) {
        audio_callback_(seconds, fps_, ost->enc->ch_layout.nb_channels, tmp);
      }

      for (int i = 0; i < ost->enc->ch_layout.nb_channels; i++) {
        *q++ = static_cast<int16_t>(tmp[i]);
      }
    }
    if (mode_ == stream_mode::HLS) {
      auto now = std::chrono::steady_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time_);
      AVCodecContext *c = ost->enc;

      ost->frame->pts = av_rescale_q(duration.count(),
                                     AVRational{1, 1000000},  // microseconds
                                     c->time_base);
      ost->next_pts = ost->frame->pts + frame->nb_samples;  // Add this line

    } else {
      ost->frame->pts = ost->next_pts;
      ost->next_pts += frame->nb_samples;
    }

    av_free(tmp);
    return frame;
  }

  /*
   * encode one audio frame and send it to the muxer
   * return 1 when encoding is finished, 0 otherwise
   */
  int write_audio_frame(AVFormatContext *oc, OutputStream *ost) {
    AVCodecContext *c;
    AVFrame *frame;
    int ret;
    int got_packet;
    int64_t dst_nb_samples;

    AVPacket *pkt = av_packet_alloc();
    if (!pkt) {
      printf("av_packet_alloc failed\n");
      return AVERROR(ENOMEM);
    }
    auto guard = sg::make_scope_guard([&] { av_packet_free(&pkt); });

    c = ost->enc;

    frame = get_audio_frame(ost);

    if (frame) {
      /* convert samples from native format to destination codec format, using the resampler */
      /* compute destination number of samples */
      dst_nb_samples = av_rescale_rnd(
          swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples, c->sample_rate, c->sample_rate, AV_ROUND_UP);
      av_assert0(dst_nb_samples == frame->nb_samples);

      /* when we pass a frame to the encoder, it may keep a reference to it
       * internally;
       * make sure we do not overwrite it here
       */
      ret = av_frame_make_writable(ost->frame);
      if (ret < 0) exit(1);

      /* convert to destination format */
      ret = swr_convert(ost->swr_ctx,
                        ost->frame->data,
                        static_cast<int>(dst_nb_samples),
                        (const uint8_t **)frame->data,
                        frame->nb_samples);
      if (ret < 0) {
        fprintf(stderr, "Error while converting\n");
        exit(1);
      }
      frame = ost->frame;

      ost->frame->pts = audio_pts;
      audio_pts += frame->nb_samples;
      audio_st.next_pts = audio_pts;

      ost->samples_count += dst_nb_samples;
    }

    ret = avcodec_send_frame(c, frame);
    if (ret < 0) {
      return ret;
    }

    ret = avcodec_receive_packet(c, pkt);
    if (ret >= 0) {
      got_packet = 1;
    } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      got_packet = 0;
      ret = 0;
    } else {
      return ret;
    }

    if (ret < 0) {
      fprintf(stderr, "Error encoding audio frame: %s\n", av_err2str(ret));
      exit(1);
    }

    if (got_packet) {
      ret = write_frame(oc, &c->time_base, ost->st, pkt);
      if (ret < 0) {
        fprintf(stderr, "Error while writing audio frame: %s\n", av_err2str(ret));
        exit(1);
      }
    }

    return (frame || got_packet) ? 0 : 1;
  }

  /**************************************************************/
  /* video output */

  AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height) {
    AVFrame *picture;
    int ret;

    picture = av_frame_alloc();
    if (!picture) return nullptr;

    picture->format = pix_fmt;
    picture->width = width;
    picture->height = height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, 32);
    if (ret < 0) {
      fprintf(stderr, "Could not allocate frame data.\n");
      exit(1);
    }

    return picture;
  }

  void open_video(AVFormatContext *oc, const AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg) {
    int ret;
    AVCodecContext *c = ost->enc;
    AVDictionary *opt = nullptr;

    av_dict_copy(&opt, opt_arg, 0);

    /* open the codec */
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
      fprintf(stderr, "Could not open video codec: %s\n", av_err2str(ret));
      exit(1);
    }

    /* allocate and init a re-usable frame */
    ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
    if (!ost->frame) {
      fprintf(stderr, "Could not allocate video frame\n");
      exit(1);
    }

    /* If the output format is not YUV420P, then a temporary YUV420P
     * picture is needed too. It is then converted to the required
     * output format. */
    ost->tmp_frame = nullptr;
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
      ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
      if (!ost->tmp_frame) {
        fprintf(stderr, "Could not allocate temporary picture\n");
        exit(1);
      }
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
      fprintf(stderr, "Could not copy the stream parameters\n");
      exit(1);
    }
  }

  void fill_yuv_image(color_mode cmode, AVFrame *pict, int frame_index, int width, int height) {
    int ret = av_frame_make_writable(pict);
    if (ret < 0) exit(1);

    size_t index = 0;
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        uint32_t &pixel((*pixels_)[index++]);

        uint8_t R, G, B;
        switch (cmode) {
          case color_mode::RGBA:
            // used by SFML
            R = (pixel & 0x000000FF) >> 0;
            G = (pixel & 0x0000FF00) >> 8;
            B = (pixel & 0x00FF0000) >> 16;
            // A = (pixel & 0xFF000000) >> 24;
            break;
          case color_mode::BGRA:
            // Used by Allegro 5
            B = (pixel & 0x000000FF) >> 0;
            G = (pixel & 0x0000FF00) >> 8;
            R = (pixel & 0x00FF0000) >> 16;
            // A = (pixel & 0xFF000000) >> 24;
            break;
        }
        // convert RGBA -> CMYK
        uint8_t Y = static_cast<uint8_t>((0.257 * R) + (0.504 * G) + (0.098 * B) + 16);
        uint8_t Cb = static_cast<uint8_t>((-0.148 * R) - (0.291 * G) + (0.439 * B) + 128);
        uint8_t Cr = static_cast<uint8_t>((0.439 * R) - (0.368 * G) - (0.071 * B) + 128);

        pict->data[0][y * pict->linesize[0] + x] = Y;
        if ((y % 2) == 0 && (x % 2) == 0) {
          pict->data[1][(y / 2) * pict->linesize[1] + (x / 2)] = Cb;
          pict->data[2][(y / 2) * pict->linesize[2] + (x / 2)] = Cr;
        }
      }
    }
  }

  AVFrame *get_video_frame(OutputStream *ost) {
    AVCodecContext *c = ost->enc;

    /* check if we want to generate more frames */
    //        if (av_compare_ts(ost->next_pts, c->time_base,
    //                          STREAM_DURATION, (AVRational){ 1, 1 }) >= 0)
    //            return nullptr;

    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
      /* as we only generate a YUV420P picture, we must convert it
       * to the codec pixel format if needed */
      if (!ost->sws_ctx) {
        ost->sws_ctx = sws_getContext(c->width,
                                      c->height,
                                      AV_PIX_FMT_YUV420P,
                                      c->width,
                                      c->height,
                                      c->pix_fmt,
                                      SCALE_FLAGS,
                                      nullptr,
                                      nullptr,
                                      nullptr);
        if (!ost->sws_ctx) {
          fprintf(stderr, "Could not initialize the conversion context\n");
          exit(1);
        }
      }
      fill_yuv_image(cmode_, ost->tmp_frame, static_cast<int>(ost->next_pts), c->width, c->height);
      sws_scale(ost->sws_ctx,
                (const uint8_t *const *)ost->tmp_frame->data,
                ost->tmp_frame->linesize,
                0,
                c->height,
                ost->frame->data,
                ost->frame->linesize);
    } else {
      fill_yuv_image(cmode_, ost->frame, static_cast<int>(ost->next_pts), c->width, c->height);
    }
    if (mode_ == stream_mode::HLS) {
      auto now = std::chrono::steady_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time_);
      ost->frame->pts = av_rescale_q(duration.count(),
                                     AVRational{1, 1000000},  // microseconds
                                     c->time_base);
      video_st.next_pts = ost->frame->pts + 1;

    } else {
      ost->frame->pts = video_pts;
      video_pts += av_rescale_q(1, AVRational{1, (int)fps_}, video_st.enc->time_base);
      video_st.next_pts = video_pts;
    }
    return ost->frame;
  }

  /*
   * encode one video frame and send it to the muxer
   * return 1 when encoding is finished, 0 otherwise
   */
  int write_video_frame(AVFormatContext *oc, OutputStream *ost) {
    int ret;
    AVCodecContext *c;
    AVFrame *frame;
    int got_packet = 0;

    c = ost->enc;

    frame = get_video_frame(ost);

    AVPacket *pkt = av_packet_alloc();
    if (!pkt) {
      printf("av_packet_alloc failed\n");
      return AVERROR(ENOMEM);
    }
    auto guard = sg::make_scope_guard([&] { av_packet_free(&pkt); });

    /* encode the image */
    ret = avcodec_send_frame(c, frame);
    if (ret < 0) {
      return ret;
    }

    ret = avcodec_receive_packet(c, pkt);

    if (ret >= 0) {
      if (pkt->duration == 0) {
        if (c->codec_type == AVMEDIA_TYPE_AUDIO) {
          pkt->duration = frame->nb_samples;
        } else if (c->codec_type == AVMEDIA_TYPE_VIDEO) {
          pkt->duration = av_rescale_q(1, c->time_base, ost->enc->time_base);
        }
      }
      got_packet = 1;
    } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      got_packet = 0;
      ret = 0;
    } else {
      return ret;
    }

    if (ret < 0) {
      fprintf(stderr, "Error encoding video frame: %s\n", av_err2str(ret));
      exit(1);
    }

    if (got_packet) {
      ret = write_frame(oc, &c->time_base, ost->st, pkt);
    } else {
      ret = 0;
    }

    if (ret < 0) {
      fprintf(stderr, "Error while writing video frame: %s\n", av_err2str(ret));
      exit(1);
    }

    return (frame || got_packet) ? 0 : 1;
  }

  void close_stream(AVFormatContext *oc, OutputStream *ost) {
    av_channel_layout_uninit(&ost->frame->ch_layout);
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    sws_freeContext(ost->sws_ctx);
    swr_free(&ost->swr_ctx);
  }

  /**************************************************************/
  /* media file output */
};

static void av_log_callback(void *ptr, int level, const char *fmt, va_list vl) {
  if (!global_this) return;

  char buf[512] = {0x00};
  vsnprintf(buf, 512, fmt, vl);
  buf[512 - 1] = 0x00;
  global_this->log_callback_level = level;
  global_this->log_callback_buffer.append(buf);
  if (global_this->log_callback_buffer[global_this->log_callback_buffer.size() - 1] == '\n') {
    global_this->log_callback(global_this->log_callback_level, global_this->log_callback_buffer);
    global_this->log_callback_buffer = "";
  }
}
