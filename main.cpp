#include <SFML/Graphics.hpp>

#include "framer.hpp"

#include <cmath>
#include <cstdlib>
#include <functional>
#include <random>

int main() {
  const int window_width = 400;
  const int window_height = 300;
  const float ball_radius = 16.f;
  const int bpp = 32;
  const bool vsync = false;

  sf::RenderWindow window(sf::VideoMode(window_width, window_height, bpp), "starcry");

  if (vsync) window.setVerticalSyncEnabled(true);

  std::random_device seed_device;
  std::default_random_engine engine(seed_device());
  std::uniform_int_distribution<int> distribution(5, 10);
  auto random = std::bind(distribution, std::ref(engine));

  sf::Vector2f direction(random(), random());
  const float velocity = std::sqrt(direction.x * direction.x + direction.y * direction.y);

  sf::CircleShape ball(ball_radius - 4);
  ball.setOutlineThickness(4);
  ball.setOutlineColor(sf::Color::Black);
  ball.setFillColor(sf::Color::Yellow);
  ball.setOrigin(ball.getRadius(), ball.getRadius());
  ball.setPosition(window_width / 2, window_height / 2);

  sf::Font font;
  font.loadFromFile("monaco.ttf");
  sf::Text text;
  text.setFont(font);         // font is a sf::Font
  text.setCharacterSize(24);  // in pixels, not points!
  text.setFillColor(sf::Color::Red);
  text.setStyle(sf::Text::Bold | sf::Text::Underlined);

  sf::Clock clock, total_clock;
  sf::Time elapsed = clock.restart();

  double fps = 25;
  const sf::Time update_ms = sf::seconds(1.f / fps);

  // frame_streamer fs("test.m3u8", frame_streamer::stream_mode::HLS);
  frame_streamer fs("test.m3u8", 10000, 30, 800, 600, frame_streamer::stream_mode::HLS);
  // frame_streamer fs("rtmp://127.0.0.1/flvplayback/video", frame_streamer::stream_mode::RTMP);
  // frame_streamer fs("test.mp4");

  fs.run();

  size_t frames = 0;

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if ((event.type == sf::Event::Closed) ||
          ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Escape))) {
        window.close();
        break;
      }
      if (event.type == sf::Event::Resized) {
        window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
      }
    }
    std::string s = std::string("FPS: ") +
                    std::to_string(static_cast<int>(frames / total_clock.getElapsedTime().asSeconds())) +
                    std::string(" TIME: " + std::to_string(static_cast<int>(total_clock.getElapsedTime().asSeconds())));
    text.setString(s);

    elapsed += clock.restart();
    while (elapsed >= update_ms) {
      const auto pos = ball.getPosition();
      // const auto delta = update_ms.asSeconds() * velocity;
      const auto delta = elapsed.asSeconds() * velocity;
      sf::Vector2f new_pos(pos.x + direction.x * delta, pos.y + direction.y * delta);

      if (new_pos.x - ball_radius < 0) {  // left window edge
        direction.x *= -1;
        new_pos.x = 0 + ball_radius;
        fs.test = true;
      } else if (new_pos.x + ball_radius >= window_width) {  // right window edge
        direction.x *= -1;
        new_pos.x = window_width - ball_radius;
        fs.test = true;
      } else if (new_pos.y - ball_radius < 0) {  // top of window
        direction.y *= -1;
        new_pos.y = 0 + ball_radius;
        fs.test = true;
      } else if (new_pos.y + ball_radius >= window_height) {  // bottom of window
        direction.y *= -1;
        new_pos.y = window_height - ball_radius;
        fs.test = true;
      }
      ball.setPosition(new_pos);

      elapsed -= update_ms;
    }

    window.clear(sf::Color(30, 30, 120));
    window.draw(ball);
    window.draw(text);
    window.display();
    frames++;

    // hack hack hack
    sf::Vector2u windowSize = window.getSize();
    if (!(windowSize.x >= window_width && windowSize.y >= window_height)) {
      continue;
    }

    sf::Texture texture;
    texture.create(windowSize.x, windowSize.y);
    texture.update(window);
    sf::Image screenshot = texture.copyToImage();
    fs.add_frame(screenshot.getPixelsPtr());
  }
  fs.finalize();

  return EXIT_SUCCESS;
}
