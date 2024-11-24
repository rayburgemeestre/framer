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

#include <SDL2/SDL.h>
#include <cmath>
#include <vector>
#include "framer.hpp"

struct AudioData {
  Uint8* pos;
  Uint8* start;
  Uint32 length;
  bool loop;
};

void audioCallback(void* userdata, Uint8* stream, int len) {
  AudioData* audio = (AudioData*)userdata;

  // Copy requested amount of data to stream
  Uint32 remaining = audio->length - (audio->pos - audio->start);
  if (len > remaining && audio->loop) {
    // Handle loop case
    memcpy(stream, audio->pos, remaining);
    memcpy(stream + remaining, audio->start, len - remaining);
    audio->pos = audio->start + (len - remaining);
  } else {
    memcpy(stream, audio->pos, len);
    audio->pos += len;
  }
}

int main() {
  const int width = 800;
  const int height = 600;
  const int fps = 30;
  const int video_seconds = 5;

  // Initialize SDL
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

  // Create window and renderer
  SDL_Window* window = SDL_CreateWindow(
      "SDL Capture", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  // Load WAV file
  SDL_AudioSpec wav_spec;
  Uint8* wav_buffer;
  Uint32 wav_length;
  if (SDL_LoadWAV("cuckoo_clock1_x.wav", &wav_spec, &wav_buffer, &wav_length) == nullptr) {
    SDL_Log("Failed to load WAV: %s", SDL_GetError());
    return 1;
  }

  // Setup audio data
  AudioData audio = {.pos = wav_buffer, .start = wav_buffer, .length = wav_length, .loop = true};

  // Setup SDL audio
  wav_spec.callback = audioCallback;
  wav_spec.userdata = &audio;
  SDL_AudioDeviceID audioDevice = SDL_OpenAudioDevice(nullptr, 0, &wav_spec, nullptr, 0);
  SDL_PauseAudioDevice(audioDevice, 0);

  // Initialize video encoder
  frame_streamer fs("sdl_capture.mp4", 100000000, fps, width, height, frame_streamer::stream_mode::FILE);

  // Set up audio callback for the video file
  // This will read from our WAV buffer directly
  fs.set_audio_callback(
      [wav_buffer, wav_length, wav_spec](float seconds, int fps, int num_channels, int16_t* channels) {
        // Calculate position in WAV buffer based on time
        Uint32 pos =
            static_cast<Uint32>(seconds * wav_spec.freq) * wav_spec.channels * (SDL_AUDIO_BITSIZE(wav_spec.format) / 8);
        pos %= wav_length;  // Loop the sample

        // Copy audio data for all channels
        for (int i = 0; i < num_channels; i++) {
          // Handle different WAV formats
          if (wav_spec.format == AUDIO_S16) {
            channels[i] = ((int16_t*)wav_buffer)[pos / 2];
          } else if (wav_spec.format == AUDIO_U8) {
            channels[i] = (wav_buffer[pos] - 128) * 256;
          }
        }
      });

  // Vector for pixel data
  std::vector<unsigned int> pixels(width * height);

  // Main loop
  for (int frame = 0; frame < fps * video_seconds; frame++) {
    float time = frame / static_cast<float>(fps);

    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw a moving circle
    int centerX = width / 2 + static_cast<int>(100 * sin(time * 2.0));
    int centerY = height / 2 + static_cast<int>(100 * cos(time * 2.0));
    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);

    for (int y = -30; y <= 30; y++) {
      for (int x = -30; x <= 30; x++) {
        if (x * x + y * y <= 900) {  // 30^2
          SDL_RenderDrawPoint(renderer, centerX + x, centerY + y);
        }
      }
    }

    SDL_RenderPresent(renderer);

    // Capture frame
    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, surface->pixels, surface->pitch);
    memcpy(pixels.data(), surface->pixels, width * height * sizeof(unsigned int));
    SDL_FreeSurface(surface);

    // Add frame to video
    fs.add_frame(pixels);

    SDL_Delay(1000 / fps);
  }

  // Cleanup
  fs.finalize();
  SDL_FreeWAV(wav_buffer);
  SDL_CloseAudioDevice(audioDevice);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
