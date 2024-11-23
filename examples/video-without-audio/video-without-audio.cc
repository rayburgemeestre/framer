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

  frame_streamer fs("video-without-audio.mp4", 100000000, fps, width, height, frame_streamer::stream_mode::FILE);
  for (int i = 0; i < fps * video_seconds; i++) {
    std::vector<unsigned int> pixels(width * height);
    float time = i * 0.1f;
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        float wave = sin(x * 0.03f + y * 0.03f + time) * 0.5f + 0.5f;
        unsigned char val = static_cast<unsigned char>(255 * wave);
        pixels[y * width + x] = (val << 24) | (val << 16) | (val << 8) | 0xFF;
      }
    }
    fs.add_frame(pixels);
  }
  fs.finalize();
  return 0;
}
