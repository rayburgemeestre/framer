#if 2 == 1
// emacs: -*- Mode: c++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// This file is Copyright (C) 2014 by Jesper Juhl <jj@chaosbits.net>
// This file may be freely used under the terms of the zlib license (http://opensource.org/licenses/Zlib)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
//    1. The origin of this software must not be misrepresented; you must not claim that you wrote
//       the original software. If you use this software in a product, an acknowledgment in the
//       product documentation would be appreciated but is not required.
//
//    2. Altered source versions must be plainly marked as such, and must not be misrepresented as
//       being the original software.
//
//    3. This notice may not be removed or altered from any source distribution.
#include <SFML/Graphics.hpp>
#include <random>
#include <functional>
#include <cstdlib>
#include <cmath>

#include "ffmpeg.hpp"
#include <thread>
#include <chrono>

int main()
{
    const int window_width = 400;
    const int window_height = 300;
    const float ball_radius = 16.f;
    const int bpp = 32;

    sf::RenderWindow window(sf::VideoMode(window_width, window_height, bpp), "Bouncing ball");
    window.setVerticalSyncEnabled(true);

    std::random_device seed_device;
    std::default_random_engine engine(seed_device());
    std::uniform_int_distribution<int> distribution(-16, 16);
    auto random = std::bind(distribution, std::ref(engine));

    sf::Vector2f direction(random(), random());
    const float velocity = std::sqrt(direction.x * direction.x + direction.y * direction.y);

    sf::CircleShape ball(ball_radius - 4);
    ball.setOutlineThickness(4);
    ball.setOutlineColor(sf::Color::Black);
    ball.setFillColor(sf::Color::Yellow);
    ball.setOrigin(ball.getRadius(), ball.getRadius());
    ball.setPosition(window_width / 2, window_height / 2);

    sf::Clock clock;
    sf::Time elapsed = clock.restart();

    double fps = 85;
    const sf::Time update_ms = sf::seconds(1.f / fps);

    ffmpeg_streamer hls("mp4", "test.mp4", 10000, fps, window_width, window_height);
    //ffmpeg_streamer hls("hls", "test.m3u8", 10000, 25, window_width, window_height);
    //ffmpeg_streamer hls("rtmp", "rtmp://127.0.0.1/flvplayback/video", 10000, 25, window_width, window_height);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if ((event.type == sf::Event::Closed) ||
                ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Escape))) {
                window.close();
                break;
            }
            if (event.type == sf::Event::Resized) {
                window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
            }
        }

        // streaming:
        //elapsed += clock.restart();
        // non-streaming:
        elapsed += update_ms;
        while (elapsed >= update_ms) {
            const auto pos = ball.getPosition();
            const auto delta = update_ms.asSeconds() * velocity;
            sf::Vector2f new_pos(pos.x + direction.x * delta, pos.y + direction.y * delta);

            if (new_pos.x - ball_radius < 0) { // left window edge
                direction.x *= -1;
                new_pos.x = 0 + ball_radius;
            } else if (new_pos.x + ball_radius >= window_width) { // right window edge
                direction.x *= -1;
                new_pos.x = window_width - ball_radius;
            } else if (new_pos.y - ball_radius < 0) { // top of window
                direction.y *= -1;
                new_pos.y = 0 + ball_radius;
            } else if (new_pos.y + ball_radius >= window_height) { // bottom of window
                direction.y *= -1;
                new_pos.y = window_height - ball_radius;
            }
            ball.setPosition(new_pos);

            elapsed -= update_ms;
        }

        window.clear(sf::Color(30, 30, 120));
        window.draw(ball);
        window.display();

        // hack hack hack
        sf::Vector2u windowSize = window.getSize();
        if (!(windowSize.x >= window_width &&
              windowSize.y >= window_height))
        {
            continue;
        }

        static sf::Texture texture;
        texture.create(windowSize.x, windowSize.y);
        texture.update(window);
        sf::Image screenshot = texture.copyToImage();
        hls.add_frame(screenshot.getPixelsPtr());
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    hls.finalize();

    return EXIT_SUCCESS;
}

#else

#define  __STDC_CONSTANT_MACROS
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

#include <iostream>

/**
 * @file
 * libavformat API example.
 *
 * Output a media file in any supported libavformat format. The default
 * codecs are used.
 * @example muxing.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

extern "C" {
  #define __STDC_CONSTANT_MACROS
  #include <libavutil/avassert.h>
  #include <libavutil/channel_layout.h>
  #include <libavutil/opt.h>
  #include <libavutil/mathematics.h>
  #include <libavutil/timestamp.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  #include <libswresample/swresample.h>
}


#define SCALE_FLAGS SWS_BICUBIC

class frame_streamer
{
private:
    int STREAM_DURATION   = 10 /* seconds */;
    int STREAM_FRAME_RATE = 25 /* 25 images/s */;
    AVPixelFormat STREAM_PIX_FMT    = AV_PIX_FMT_YUV420P /* default pix_fmt */;

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

    OutputStream video_st = { nullptr }, audio_st = { nullptr };
    AVOutputFormat *fmt;
    AVFormatContext *oc;
    AVCodec *audio_codec, *video_codec;
    int ret;
    int have_video = 0, have_audio = 0;
    int encode_video = 0, encode_audio = 0;
    AVDictionary *opt = nullptr;

public:

    enum class stream_mode {
        FILE,
        RTMP,
        HLS
    };

    // todo private:
    stream_mode mode_;
    std::string filename_;


    frame_streamer(std::string filename, stream_mode mode = stream_mode::FILE)
        : mode_(mode), filename_(std::move(filename))
    {
    }

    int run()
    {
        /* Initialize libavcodec, and register all codecs and formats. */
        av_register_all();

        // for (int i = 2; i+1 < argc; i+=2) {
        //     if (!strcmp(argv[i], "-flags") || !strcmp(argv[i], "-fflags"))
        //         av_dict_set(&opt, argv[i]+1, argv[i+1], 0);
        // }

        /* allocate the output media context */
        avformat_alloc_output_context2(&oc, nullptr, nullptr, filename.c_str());
        if (!oc) {
            printf("Could not deduce output format from file extension: using MPEG.\n");
            avformat_alloc_output_context2(&oc, nullptr, "mpeg", filename.c_str());
        }
        if (!oc)
            return 1;

        fmt = oc->oformat;

        /* Add the audio and video streams using the default format codecs
         * and initialize the codecs. */
        if (fmt->video_codec != AV_CODEC_ID_NONE) {
            add_stream(&video_st, oc, &video_codec, fmt->video_codec);
            have_video = 1;
            encode_video = 1;
        }
        if (fmt->audio_codec != AV_CODEC_ID_NONE) {
            add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec);
            have_audio = 1;
            encode_audio = 1;
        }

        /* Now that all the parameters are set, we can open the audio and
         * video codecs and allocate the necessary encode buffers. */
        if (have_video)
            open_video(oc, video_codec, &video_st, opt);

        if (have_audio) {
            std::cout << "have audio" << std::endl;
            open_audio(oc, audio_codec, &audio_st, opt);
        }

        av_dump_format(oc, 0, filename.c_str(), 1);

        /* open the output file, if needed */
        if (!(fmt->flags & AVFMT_NOFILE)) {
            ret = avio_open(&oc->pb, filename.c_str(), AVIO_FLAG_WRITE);
            if (ret < 0) {
                fprintf(stderr, "Could not open '%s': %s\n", filename.c_str(),
                        av_err2str(ret));
                return 1;
            }
        }

        /* Write the stream header, if any. */
        ret = avformat_write_header(oc, &opt);
        if (ret < 0) {
            fprintf(stderr, "Error occurred when opening output file: %s\n",
                    av_err2str(ret));
            return 1;
        }

        while (encode_video || encode_audio) {
            /* select the stream to encode */
            if (encode_video &&
                (!encode_audio || av_compare_ts(video_st.next_pts, video_st.enc->time_base,
                                                audio_st.next_pts, audio_st.enc->time_base) <= 0)) {
                encode_video = !write_video_frame(oc, &video_st);
            } else {
                encode_audio = !write_audio_frame(oc, &audio_st);
            }
        }

        /* Write the trailer, if any. The trailer must be written before you
         * close the CodecContexts open when you wrote the header; otherwise
         * av_write_trailer() may try to use memory that was freed on
         * av_codec_close(). */
        av_write_trailer(oc);

        /* Close each codec. */
        if (have_video)
            close_stream(oc, &video_st);
        if (have_audio)
            close_stream(oc, &audio_st);

        if (!(fmt->flags & AVFMT_NOFILE))
            /* Close the output file. */
            avio_closep(&oc->pb);

        /* free the stream */
        // Why does this shit crash??
        // avformat_free_context(oc);

        return 0;
    }


private:

    void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
    {
        AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

        printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
               av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
               av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
               av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
               pkt->stream_index);
    }

    int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
    {
        /* rescale output packet timestamp values from codec to stream timebase */
        av_packet_rescale_ts(pkt, *time_base, st->time_base);
        pkt->stream_index = st->index;

        /* Write the compressed frame to the media file. */
        log_packet(fmt_ctx, pkt);
        return av_interleaved_write_frame(fmt_ctx, pkt);
    }

/* Add an output stream. */
    void add_stream(OutputStream *ost, AVFormatContext *oc,
                           AVCodec **codec,
                           enum AVCodecID codec_id)
    {
        AVCodecContext *c;
        int i;

        /* find the encoder */
        *codec = avcodec_find_encoder(codec_id);
        if (!(*codec)) {
            fprintf(stderr, "Could not find encoder for '%s'\n",
                    avcodec_get_name(codec_id));
            exit(1);
        }

        ost->st = avformat_new_stream(oc, nullptr);
        if (!ost->st) {
            fprintf(stderr, "Could not allocate stream\n");
            exit(1);
        }
        ost->st->id = oc->nb_streams-1;
        c = avcodec_alloc_context3(*codec);
        if (!c) {
            fprintf(stderr, "Could not alloc an encoding context\n");
            exit(1);
        }
        ost->enc = c;

        switch ((*codec)->type) {
            case AVMEDIA_TYPE_AUDIO:
                c->sample_fmt  = (*codec)->sample_fmts ?
                                 (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
                c->bit_rate    = 64000;
                c->sample_rate = 44100;
                if ((*codec)->supported_samplerates) {
                    c->sample_rate = (*codec)->supported_samplerates[0];
                    for (i = 0; (*codec)->supported_samplerates[i]; i++) {
                        if ((*codec)->supported_samplerates[i] == 44100)
                            c->sample_rate = 44100;
                    }
                }
                c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
                c->channel_layout = AV_CH_LAYOUT_STEREO;
                if ((*codec)->channel_layouts) {
                    c->channel_layout = (*codec)->channel_layouts[0];
                    for (i = 0; (*codec)->channel_layouts[i]; i++) {
                        if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                            c->channel_layout = AV_CH_LAYOUT_STEREO;
                    }
                }
                c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
                ost->st->time_base = (AVRational){ 1, c->sample_rate };
                break;

            case AVMEDIA_TYPE_VIDEO:
                c->codec_id = codec_id;

                c->bit_rate = 400000;
                /* Resolution must be a multiple of two. */
                c->width    = 352;
                c->height   = 288;
                /* timebase: This is the fundamental unit of time (in seconds) in terms
                 * of which frame timestamps are represented. For fixed-fps content,
                 * timebase should be 1/framerate and timestamp increments should be
                 * identical to 1. */
                ost->st->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
                c->time_base       = ost->st->time_base;

                c->gop_size      = 12; /* emit one intra frame every twelve frames at most */
                c->pix_fmt       = STREAM_PIX_FMT;
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
        if (oc->oformat->flags & AVFMT_GLOBALHEADER)
            c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

/**************************************************************/
/* audio output */

    AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                      uint64_t channel_layout,
                                      int sample_rate, int nb_samples)
    {
        AVFrame *frame = av_frame_alloc();
        int ret;

        if (!frame) {
            fprintf(stderr, "Error allocating an audio frame\n");
            exit(1);
        }

        frame->format = sample_fmt;
        frame->channel_layout = channel_layout;
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

    void open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
    {
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
        ost->t     = 0;
        ost->tincr = static_cast<float>(2 * M_PI * 110.0 / c->sample_rate);
        /* increment frequency by 110 Hz per second */
        ost->tincr2 = static_cast<float>(2 * M_PI * 110.0 / c->sample_rate / c->sample_rate);

        if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
            nb_samples = 10000;
        else
            nb_samples = c->frame_size;

        ost->frame     = alloc_audio_frame(c->sample_fmt, c->channel_layout,
                                           c->sample_rate, nb_samples);
        ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, c->channel_layout,
                                           c->sample_rate, nb_samples);

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
        av_opt_set_int       (ost->swr_ctx, "in_channel_count",   c->channels,       0);
        av_opt_set_int       (ost->swr_ctx, "in_sample_rate",     c->sample_rate,    0);
        av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
        av_opt_set_int       (ost->swr_ctx, "out_channel_count",  c->channels,       0);
        av_opt_set_int       (ost->swr_ctx, "out_sample_rate",    c->sample_rate,    0);
        av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt",     c->sample_fmt,     0);

        /* initialize the resampling context */
        if ((ret = swr_init(ost->swr_ctx)) < 0) {
            fprintf(stderr, "Failed to initialize the resampling context\n");
            exit(1);
        }
    }

/* Prepare a 16 bit dummy audio frame of 'frame_size' samples and
 * 'nb_channels' channels. */
    AVFrame *get_audio_frame(OutputStream *ost)
    {
        AVFrame *frame = ost->tmp_frame;
        int j, i, v;
        auto *q = (int16_t*)frame->data[0];

        /* check if we want to generate more frames */
        if (av_compare_ts(ost->next_pts, ost->enc->time_base,
                          STREAM_DURATION, (AVRational){ 1, 1 }) >= 0)
            return nullptr;

        for (j = 0; j <frame->nb_samples; j++) {
            v = (int)(sin(ost->t) * 10000);
            for (i = 0; i < ost->enc->channels; i++)
                *q++ = static_cast<int16_t>(v);
            ost->t     += ost->tincr;
            ost->tincr += ost->tincr2;
        }

        frame->pts = ost->next_pts;
        ost->next_pts  += frame->nb_samples;

        return frame;
    }

/*
 * encode one audio frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
    int write_audio_frame(AVFormatContext *oc, OutputStream *ost)
    {
        AVCodecContext *c;
        AVPacket pkt = { nullptr }; // data and size must be 0;
        AVFrame *frame;
        int ret;
        int got_packet;
        int64_t dst_nb_samples;

        av_init_packet(&pkt);
        c = ost->enc;

        frame = get_audio_frame(ost);

        if (frame) {
            /* convert samples from native format to destination codec format, using the resampler */
            /* compute destination number of samples */
            dst_nb_samples = av_rescale_rnd(swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples,
                                            c->sample_rate, c->sample_rate, AV_ROUND_UP);
            av_assert0(dst_nb_samples == frame->nb_samples);

            /* when we pass a frame to the encoder, it may keep a reference to it
             * internally;
             * make sure we do not overwrite it here
             */
            ret = av_frame_make_writable(ost->frame);
            if (ret < 0)
                exit(1);

            /* convert to destination format */
            ret = swr_convert(ost->swr_ctx,
                              ost->frame->data, static_cast<int>(dst_nb_samples),
                              (const uint8_t **)frame->data, frame->nb_samples);
            if (ret < 0) {
                fprintf(stderr, "Error while converting\n");
                exit(1);
            }
            frame = ost->frame;

            frame->pts = av_rescale_q(ost->samples_count, (AVRational){1, c->sample_rate}, c->time_base);
            ost->samples_count += dst_nb_samples;
        }

        ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
        if (ret < 0) {
            fprintf(stderr, "Error encoding audio frame: %s\n", av_err2str(ret));
            exit(1);
        }

        if (got_packet) {
            ret = write_frame(oc, &c->time_base, ost->st, &pkt);
            if (ret < 0) {
                fprintf(stderr, "Error while writing audio frame: %s\n",
                        av_err2str(ret));
                exit(1);
            }
        }

        return (frame || got_packet) ? 0 : 1;
    }

/**************************************************************/
/* video output */

    AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
    {
        AVFrame *picture;
        int ret;

        picture = av_frame_alloc();
        if (!picture)
            return nullptr;

        picture->format = pix_fmt;
        picture->width  = width;
        picture->height = height;

        /* allocate the buffers for the frame data */
        ret = av_frame_get_buffer(picture, 32);
        if (ret < 0) {
            fprintf(stderr, "Could not allocate frame data.\n");
            exit(1);
        }

        return picture;
    }

    void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
    {
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

/* Prepare a dummy image. */
    void fill_yuv_image(AVFrame *pict, int frame_index,
                               int width, int height)
    {
        int x, y, i, ret;

        /* when we pass a frame to the encoder, it may keep a reference to it
         * internally;
         * make sure we do not overwrite it here
         */
        ret = av_frame_make_writable(pict);
        if (ret < 0)
            exit(1);

        i = frame_index;

        /* Y */
        for (y = 0; y < height; y++)
            for (x = 0; x < width; x++)
                pict->data[0][y * pict->linesize[0] + x] = static_cast<uint8_t>(x + y + i * 3);

        /* Cb and Cr */
        for (y = 0; y < height / 2; y++) {
            for (x = 0; x < width / 2; x++) {
                pict->data[1][y * pict->linesize[1] + x] = static_cast<uint8_t>(128 + y + i * 2);
                pict->data[2][y * pict->linesize[2] + x] = static_cast<uint8_t>(64 + x + i * 5);
            }
        }
    }

    AVFrame *get_video_frame(OutputStream *ost)
    {
        AVCodecContext *c = ost->enc;

        /* check if we want to generate more frames */
        if (av_compare_ts(ost->next_pts, c->time_base,
                          STREAM_DURATION, (AVRational){ 1, 1 }) >= 0)
            return nullptr;

        if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
            /* as we only generate a YUV420P picture, we must convert it
             * to the codec pixel format if needed */
            if (!ost->sws_ctx) {
                ost->sws_ctx = sws_getContext(c->width, c->height,
                                              AV_PIX_FMT_YUV420P,
                                              c->width, c->height,
                                              c->pix_fmt,
                                              SCALE_FLAGS, nullptr, nullptr, nullptr);
                if (!ost->sws_ctx) {
                    fprintf(stderr,
                            "Could not initialize the conversion context\n");
                    exit(1);
                }
            }
            fill_yuv_image(ost->tmp_frame, static_cast<int>(ost->next_pts), c->width, c->height);
            sws_scale(ost->sws_ctx,
                      (const uint8_t * const *)ost->tmp_frame->data, ost->tmp_frame->linesize,
                      0, c->height, ost->frame->data, ost->frame->linesize);
        } else {
            fill_yuv_image(ost->frame, static_cast<int>(ost->next_pts), c->width, c->height);
        }

        ost->frame->pts = ost->next_pts++;

        return ost->frame;
    }

/*
 * encode one video frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
    int write_video_frame(AVFormatContext *oc, OutputStream *ost)
    {
        int ret;
        AVCodecContext *c;
        AVFrame *frame;
        int got_packet = 0;
        AVPacket pkt = { nullptr };

        c = ost->enc;

        frame = get_video_frame(ost);

        av_init_packet(&pkt);

        /* encode the image */
        ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
        if (ret < 0) {
            fprintf(stderr, "Error encoding video frame: %s\n", av_err2str(ret));
            exit(1);
        }

        if (got_packet) {
            ret = write_frame(oc, &c->time_base, ost->st, &pkt);
        } else {
            ret = 0;
        }

        if (ret < 0) {
            fprintf(stderr, "Error while writing video frame: %s\n", av_err2str(ret));
            exit(1);
        }

        return (frame || got_packet) ? 0 : 1;

    }

    void close_stream(AVFormatContext *oc, OutputStream *ost)
    {
        avcodec_free_context(&ost->enc);
        av_frame_free(&ost->frame);
        av_frame_free(&ost->tmp_frame);
        sws_freeContext(ost->sws_ctx);
        swr_free(&ost->swr_ctx);
    }

/**************************************************************/
/* media file output */
};



int main(int argc, char **argv)
{
    frame_streamer fs("output.mp4");

    fs.run();


}
#endif
