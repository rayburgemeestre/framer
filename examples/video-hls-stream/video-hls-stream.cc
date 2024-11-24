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
#include <cstdlib>
#include <functional>
#include <thread>

float smooth_envelope(float t, float attack, float release, float total_duration);
std::vector<unsigned int> cool_gradient_spiral_animation(float time, int width, int height);

int main() {
  bool is_smoke_test = std::getenv("SMOKE_TEST") != nullptr;
  const int fps = 30;
  const int video_seconds = is_smoke_test ? 5 : 60;
  const int width = 400;
  const int height = 300;

  frame_streamer fs("test_stream.m3u8", 1000000, fps, width, height, frame_streamer::stream_mode::HLS);

  fs.set_audio_callback([](float seconds, int fps, int num_channels, int16_t *channels) {
    float frequency = 369.0f;    // Hz
    float amplitude = 10000.0f;  // Volume

    // Configuration for the envelope
    const float beep_duration = 0.1f;  // Duration of each beep
    const float attack_time = 0.002f;  // 10ms attack
    const float release_time = 0.01f;  // 50ms release

    // Calculate time within the current second
    float time_in_cycle = fmod(seconds, 1.0f);

    int v = 0;
    if (time_in_cycle < beep_duration) {
      // Get envelope multiplier (0.0 to 1.0)
      float env = smooth_envelope(time_in_cycle, attack_time, release_time, beep_duration);

      // Apply envelope to the sine wave
      v = (int)(amplitude * env * sin(2 * M_PI * frequency * seconds));
    }

    for (int i = 0; i < num_channels; i++) {
      *channels++ = static_cast<int16_t>(v);
    }
  });

  // Below simple code works, but it pushes out frames as fast as it can.
  // This may not be what you want!
  /*
  for (int i = 0; i < fps * video_seconds; i++) {
      auto pixelVector = cool_gradient_spiral_animation(i * 0.1f, width, height);
      fs.add_frame(pixelVector);
  }
  */

  // Below code generates frames in real-time by tracking elapsed time:
  // - If running too slow: skips frames to catch up
  // - If running too fast: sleeps to maintain correct frame rate
  // This ensures the animation streams at the intended speed regardless of generation performance
  auto stream_start = std::chrono::steady_clock::now();
  const double frame_duration = 1.0 / fps;  // Duration of one frame in seconds
  for (int i = 0; i < fps * video_seconds; i++) {
    // Calculate target time for this frame
    double target_time = i * frame_duration;
    // Get actual elapsed time
    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - stream_start).count();
    if (elapsed > target_time + frame_duration) {
      // We're running too slow - skip this frame
      continue;
    }
    if (elapsed < target_time) {
      // We're running too fast - sleep until target time
      std::this_thread::sleep_for(std::chrono::duration<double>(target_time - elapsed));
    }
    auto pixelVector = cool_gradient_spiral_animation(i * 0.1f, width, height);
    fs.add_frame(pixelVector);
  }

  fs.finalize();
}

float smooth_envelope(float t, float attack, float release, float total_duration) {
  if (t < 0.0f || t > total_duration) {
    return 0.0f;
  }

  // Attack phase with sine-based smoothing
  if (t < attack) {
    return (1.0f - cos(M_PI * t / attack)) * 0.5f;
  }

  // Release phase with sine-based smoothing
  if (t > (total_duration - release)) {
    float release_t = (total_duration - t) / release;
    return (1.0f - cos(M_PI * release_t)) * 0.5f;
  }

  // Sustain phase
  return 1.0f;
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