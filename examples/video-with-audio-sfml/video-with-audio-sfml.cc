#include "framer.hpp"

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

int main() {
  const int FPS = 30;
  const int WIDTH = 800;
  const int HEIGHT = 600;

  frame_streamer fs("video-audio-sfml.mp4", 100000000, FPS, WIDTH, HEIGHT, frame_streamer::stream_mode::FILE);

  // Load audio file
  sf::SoundBuffer buffer;
  if (!buffer.loadFromFile("cuckoo_clock1_x.wav")) {  // Replace with your audio file
    std::cerr << "Failed to load audio file!" << std::endl;
    return -1;
  }

  // Store audio data we need before creating the sound
  const sf::Int16* samples = buffer.getSamples();
  const size_t sampleCount = buffer.getSampleCount();
  const size_t sampleRate = buffer.getSampleRate();
  const size_t channelCount = buffer.getChannelCount();
  const float duration = buffer.getDuration().asSeconds();

  // Set up audio callback for the frame streamer
  fs.set_audio_callback(
      [samples, sampleRate, channelCount, duration](float seconds, int fps, int num_channels, int16_t* channels) {
        float currentTime = fmod(seconds, duration);
        size_t samplePos = static_cast<size_t>(currentTime * sampleRate);

        for (int i = 0; i < num_channels; i++) {
          channels[i] = samples[samplePos * channelCount + (i % channelCount)];
        }
      });

  // Create render texture
  sf::RenderTexture renderTexture;
  if (!renderTexture.create(WIDTH, HEIGHT)) {
    std::cerr << "Failed to create render texture!" << std::endl;
    return -1;
  }

  // Create circles for visualization
  const int NUM_CIRCLES = 8;
  std::vector<sf::CircleShape> circles(NUM_CIRCLES);
  for (int i = 0; i < NUM_CIRCLES; ++i) {
    circles[i].setRadius(20.f);
    circles[i].setFillColor(sf::Color(255, 64 + i * 24, 64 + i * 24));
  }

  // Render frames
  for (int frame = 1; frame <= FPS * 10; ++frame) {  // 10 seconds of video
    // Get current audio amplitude
    float seconds = static_cast<float>(frame) / FPS;
    size_t samplePos = static_cast<size_t>(fmod(seconds, duration) * sampleRate);

    // Calculate average amplitude over a small window
    float amplitude = 0.0f;
    const size_t windowSize = 500;
    for (size_t i = 0; i < windowSize && (samplePos + i) < sampleCount; ++i) {
      amplitude += std::abs(static_cast<float>(samples[samplePos + i])) / 32768.0f;
    }
    amplitude = amplitude / windowSize * 100.0f;  // Scale for visibility

    renderTexture.clear(sf::Color(20, 20, 20));  // Dark background

    // Update and draw circles
    for (int i = 0; i < NUM_CIRCLES; ++i) {
      float angle = (frame * 2.0f + i * (360.0f / NUM_CIRCLES)) * 3.14159f / 180.0f;
      float radius = 100.0f + amplitude * 2.0f;
      float x = WIDTH / 2 + std::cos(angle) * radius;
      float y = HEIGHT / 2 + std::sin(angle) * radius;

      circles[i].setPosition(x - circles[i].getRadius(), y - circles[i].getRadius());
      circles[i].setRadius(20.0f + amplitude * 0.2f);
      renderTexture.draw(circles[i]);
    }

    renderTexture.display();

    // Convert to ARGB format
    sf::Image screenshot = renderTexture.getTexture().copyToImage();
    const sf::Uint8* pixels = screenshot.getPixelsPtr();
    std::vector<sf::Uint8> argbPixels(WIDTH * HEIGHT * 4);

    for (int i = 0; i < WIDTH * HEIGHT; ++i) {
      argbPixels[i * 4 + 3] = 255;
      argbPixels[i * 4 + 2] = pixels[i * 4 + 2];
      argbPixels[i * 4 + 1] = pixels[i * 4 + 1];
      argbPixels[i * 4 + 0] = pixels[i * 4 + 0];
    }

    fs.add_frame(argbPixels.data(), WIDTH, HEIGHT);
  }

  fs.finalize();
  return 0;
}