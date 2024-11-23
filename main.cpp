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
}
