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

#include "framer.hpp"

#include <cmath>
#include <vector>

std::vector<unsigned int> cool_gradient_spiral_animation(float time, int width, int height);

int main() {
  const int fps = 30;
  const int width = 800;
  const int height = 600;
  const int video_seconds = 5;

  frame_streamer fs("video-and-audio.mp4", 100000000, fps, width, height, frame_streamer::stream_mode::FILE);

  // beep every second
  fs.set_audio_callback([](float seconds, int fps, int num_channels, int16_t *channels) {
    float frequency = 369.0f;    // Hz
    float amplitude = 10000.0f;  // Volume
    // float current_frame = seconds * fps;
    int v = 0;
    if (fmod(seconds, 1.0f) < 0.5f) {
      v = (int)(amplitude * sin(2 * M_PI * frequency * seconds));
    }
    for (int i = 0; i < num_channels; i++) {
      *channels++ = static_cast<int16_t>(v);
    }
  });

  // slightly nicer video effect
  for (int i = 0; i < fps * video_seconds; i++) {
    auto pixelVector = cool_gradient_spiral_animation(i * 0.1f, width, height);
    fs.add_frame(pixelVector);
  }
  fs.finalize();
  return 0;
}

std::vector<unsigned int> cool_gradient_spiral_animation(float time, int width, int height) {
  std::vector<unsigned int> pixelVector;
  pixelVector.reserve(width * height);

  // Create a moving gradient pattern
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      float cx = x - width / 2;
      float cy = y - height / 2;
      float dist = sqrt(cx * cx + cy * cy);
      float angle = atan2(cy, cx);

      // Create spinning spiral pattern
      float spiral = (dist * 0.03f + angle * 2 + time) * 3;
      float wave = sin(spiral) * 0.5f + 0.5f;

      // Add pulsing circles
      float circle1 = sin(dist * 0.05f - time * 2) * 0.5f + 0.5f;
      float circle2 = sin(dist * 0.03f + time) * 0.5f + 0.5f;

      unsigned char r = static_cast<unsigned char>(255 * (wave * circle1));
      unsigned char g = static_cast<unsigned char>(255 * (wave * 0.5f + circle2 * 0.5f));
      unsigned char b = static_cast<unsigned char>(255 * circle2);

      unsigned int pixel = (r << 24) | (g << 16) | (b << 8) | 0xFF;
      pixelVector.push_back(pixel);
    }
  }
  return pixelVector;
}
