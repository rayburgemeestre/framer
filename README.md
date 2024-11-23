      F       R AMER
    [fFmpeg stReAMER]

## Building

This is a header only project, please refer to the example directories for actual examples.
These include video, audio and various streaming examples. As well as an example that shows
how to use framer with SFML.

There is also an example that shows how to use do static linking of ffmpeg in order to avoid
the need for ffmpeg libraries on the system.

## Usage in your own projects

At least this is how I use it..

First add as submodule:

    git submodule add https://github.com/rayburgemeestre/framer libs/framer

Then add path in your `CMakeLists.txt`

    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/libs/framer/")

Use in source code (example):

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

## Notes

For streaming examples, start a webserver in the current dir, something like:

    php -S 0.0.0.0:8080

Use http://localhost:8080/test.m3u8 with vlc or check the `index.html` file for
a web player.