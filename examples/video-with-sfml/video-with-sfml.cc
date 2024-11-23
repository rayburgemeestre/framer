#include "framer.hpp"

#include <SFML/Graphics.hpp>

#include <iostream>
#include <string>

int main() {
  const int FPS = 30;
  const int WIDTH = 800;
  const int HEIGHT = 600;

  frame_streamer fs("video-sfml.mp4", 100000000, FPS, WIDTH, HEIGHT, frame_streamer::stream_mode::FILE);

  // Create a render texture (off-screen buffer) with desired dimensions
  sf::RenderTexture renderTexture;
  if (!renderTexture.create(WIDTH, HEIGHT)) {
    std::cerr << "Failed to create render texture!" << std::endl;
    return -1;
  }

  // Create a circle that we'll animate
  sf::CircleShape circle(30.f);  // 30 pixel radius
  circle.setFillColor(sf::Color::Red);

  // Animation parameters
  float xPos = 0.f;
  const float speed = 8.f;

  for (int frame = 1; frame <= FPS * 5; ++frame) {
    // Clear the render texture with white background
    renderTexture.clear(sf::Color::White);

    // Update circle position
    xPos += speed;
    if (xPos > WIDTH) {
      xPos = 0;
    }
    circle.setPosition(xPos, HEIGHT / 2 - 30);

    // Draw the circle to the render texture
    renderTexture.draw(circle);
    renderTexture.display();

    // Get the texture from the render texture
    sf::Image screenshot = renderTexture.getTexture().copyToImage();

    // Convert to ARGB format (SFML uses BGRA by default)
    const sf::Uint8* pixels = screenshot.getPixelsPtr();
    std::vector<sf::Uint8> argbPixels(WIDTH * HEIGHT * 4);

    for (int i = 0; i < WIDTH * HEIGHT; ++i) {
      // BGRA to ARGB conversion
      argbPixels[i * 4 + 0] = 255;                // Alpha (setting to fully opaque)
      argbPixels[i * 4 + 1] = pixels[i * 4 + 2];  // Red (from Blue position)
      argbPixels[i * 4 + 2] = pixels[i * 4 + 1];  // Green
      argbPixels[i * 4 + 3] = pixels[i * 4 + 0];  // Blue (from Red position)
    }

    fs.add_frame(argbPixels.data(), WIDTH, HEIGHT);
  }
  fs.finalize();
  return 0;
}