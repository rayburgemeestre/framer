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

int main() {
  const int fps = 30;
  const int width = 800;
  const int height = 600;
  const int video_seconds = 5;

  frame_streamer fs("hello_world.mp4", 100000000, fps, width, height, frame_streamer::stream_mode::FILE);

  // Simple 440 Hz tone
  fs.set_audio_callback([](float seconds, int fps, int num_channels, int16_t *channels) {
    int v = 5000 * (fmod(seconds * 440 * 2, 2) < 1 ? 1 : -1);  // square wave
    for (int i = 0; i < num_channels; i++) {
      *channels++ = static_cast<int16_t>(v);
    }
  });

  // Generate solid gray frames - using ARGB format (0xFF7F7F7F)
  std::vector<unsigned int> pixels(width * height, 0xFF7F7F7F);
  for (int i = 0; i < fps * video_seconds; i++) {
    fs.add_frame(pixels);
  }

  fs.finalize();
  return 0;
}