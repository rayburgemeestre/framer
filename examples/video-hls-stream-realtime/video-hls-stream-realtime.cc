#include "framer.hpp"

#include <cmath>
#include <cstdlib>
#include <functional>
#include <thread>

int main() {
  const int fps = 30;
  const int video_seconds = 60;
  const int width = 400;
  const int height = 300;

  frame_streamer fs("test_stream.m3u8", 1000000, fps, width, height, frame_streamer::stream_mode::HLS);

  fs.set_audio_callback([](float seconds, int fps, int num_channels, int16_t *channels) {
    int v = 5000 * (fmod(seconds * 440 * 2, 2) < 1 ? 1 : -1);
    for (int i = 0; i < num_channels; i++) {
      *channels++ = static_cast<int16_t>(v);
    }
  });

  // Compared to the video-hls-stream.cc example this code can be quite a bit simpler.
  // (No required frame skipping or extra delays on the callers side.)
  auto start = std::chrono::high_resolution_clock::now();
  fs.set_video_callback([start](std::vector<unsigned int> &pixels, int width, int height) {
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
    float seconds = elapsed / 1000.0f;
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        float wave = sin(x * 0.03f + y * 0.03f + seconds) * 0.5f + 0.5f;
        unsigned char val = static_cast<unsigned char>(255 * wave);
        pixels[y * width + x] = (val << 24) | (val << 16) | (val << 8) | 0xFF;
      }
    }
    // fake delay
  });

  // Start background thread that invokes fs.stop_loop() after some time
  std::thread stop_thread([&fs, &video_seconds]() {
    std::this_thread::sleep_for(std::chrono::seconds(video_seconds));
    fs.stop_loop();
  });

  fs.run_loop();

  fs.finalize();

  stop_thread.join();
}