      F       R AMER
    [fFmpeg stReAMER]

## Building

For Ubuntu 18.04 I made a helper docker container to assist in building to
avoid having conflicts with system ffmpeg stuff.

    make libs  # to build ffmpeg and x264 codec (and installs this to `libs`)

    make example  # build the example program (not build within docker)

Example is based on:

https://github.com/SFML/SFML/wiki/Source:-Bouncing-ball

## Running

    ./example

## Usage in projects

At least this is how I use it..

First add as submodule:

    git submodule add https://github.com/rayburgemeestre/framer libs/framer

Then add path in your `CMakeLists.txt`

    "${CMAKE_CURRENT_SOURCE_DIR}/libs/framer/"

Use in source code:

    #include "framer.hpp"

    frame_streamer fs("test.m3u8", frame_streamer::stream_mode::HLS);

    // add frames using SFML: sf::Image
    sf::Image screenshot = texture.copyToImage();
    fs.add_frame(screenshot.getPixelsPtr());

    // add frames using pixels from: std::vector<uint32_t>&
    fs.add_frame(pixels);

    fs.finalize();

Start webserver to host test.m3u8, for example:

    php -S 0.0.0.0:8080

Use http://localhost:8080/test.m3u8 with vlc or check the `index.html` file for
a web player.

## Known issues

I've tried for a long time but for now given up on supporting an audio channel.
It would be fun to generate some sound effects whenever the ball hits a wall
for example however I struggled to get this stuff in sync.
With an old ffmpeg version I managed to get this interleaving working, but I
don't want to downgrade ffmpeg for this feature right now.

