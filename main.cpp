#include "random.hpp"
#include <SFML/Graphics.hpp>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace config {
uint32_t render_width{256}, render_height{144};
uint32_t window_width{1280}, window_height{720};
std::string_view window_title{"Rocketman"};
float gap{10.0f};
float top{gap};
float bottom{render_height - gap};

float player_width{8};
float player_height{8};

float spike_velocity{0.0f};
float spike_acceleration{2e-3};
float player_velocity{2e-1f};

namespace color_palette {
sf::Color red{0xFF4649FF};
sf::Color orange{0xFF861DFF};
sf::Color gray{0x1A1A1AFF};
sf::Color white{0xFFFFFFFF};
sf::Color blue{0x55AAFFFF};
}; // namespace color_palette
}; // namespace config

struct Spike {
  sf::Vertex v1{}, v2{}, v3{};
  bool top{}, passed{};
};

std::vector<Spike> spikes{};

constexpr float degrees_to_radians(float deg) { return deg * M_PI / 180; }

void generate_spike(float x, float base_width, float height, bool top = false) {

  if (top) {
    spikes.push_back(
        Spike{sf::Vertex{sf::Vector2f{x - base_width / 2, config::top},
                         sf::Color{config::color_palette::orange}},
              sf::Vertex{sf::Vector2f{x, config::top + height},
                         sf::Color{config::color_palette::orange}},
              sf::Vertex{sf::Vector2f{x + base_width / 2, config::top},
                         sf::Color{config::color_palette::orange}},
              top});
  } else {
    spikes.push_back(
        Spike{sf::Vertex{sf::Vector2f{x - base_width / 2, config::bottom},
                         sf::Color{config::color_palette::orange}},
              sf::Vertex{sf::Vector2f{x, config::bottom - height},
                         sf::Color{config::color_palette::orange}},
              sf::Vertex{sf::Vector2f{x + base_width / 2, config::bottom},
                         sf::Color{config::color_palette::orange}},
              top});
  }
}

struct Player {
  float x{}, y{};
  float delta_x{}, delta_y{};
  bool dead{};
  sf::Texture texture{};

  void update(float delta_ms) {
    x += delta_x * delta_ms;
    y += delta_y * delta_ms;

    if (y < config::top)
      dead = true;
    else if ((y + config::player_height) > config::bottom)
      dead = true;
  }

  bool is_dead() const { return dead; }
  void is_dead(bool dead_) { dead = dead_; }
};

struct Particle {
  sf::Vector2f position{};
  sf::Vector2f size{};
  sf::Vector2f velocity{};
  sf::Vector2f acceleration{};
  float decay_rate{};
  float lifespan{255.0f};

  void update(float delta_ms) {
    position += velocity * delta_ms;
    velocity += acceleration * delta_ms;
    lifespan -= decay_rate * delta_ms;
  }

  bool is_dead() const { return lifespan <= 0.0f; }
};

struct ParticleSystem {
  std::vector<Particle> particles{};
  std::vector<sf::Vertex> vertex_array{};
  sf::Vector2f position{};
  sf::Color color;
  bool loop{};

  static ParticleSystem create(sf::Vector2f position, uint32_t n,
                               sf::Color color, bool loop = false) {
    ParticleSystem ps{};
    ps.position = position;
    ps.color = color;
    ps.loop = loop;
    for (uint32_t i{}; i < n; ++i) {
      ps.particles.push_back(Particle{
          position + sf::Vector2f{0.0f, rng::f32(0.0f, 7.0f)},
          sf::Vector2f{1.0f, 1.0f},
          sf::Vector2f{rng::f32(-0.05f, -0.01f), rng::f32(0.0f, 0.01f)},
          sf::Vector2f{-1e-7f, 0.0f}, 0.1f, rng::f32(0.0f, 255.0f)});
    }

    return ps;
  }

  void set_position(sf::Vector2f position) { this->position = position; }

  void update(float delta_ms) {
    for (auto &particle : particles) {
      if (loop) {
        if (particle.is_dead()) {
          particle = Particle{
              position + sf::Vector2f{0.0f, rng::f32(0.0f, 7.0f)},
              sf::Vector2f{1.0f, 1.0f},
              sf::Vector2f{rng::f32(-0.05f, -0.01f), rng::f32(0.0f, 0.0f)},
              sf::Vector2f{-1e-5f, 0.0f},
              0.1f,
              rng::f32(0.0f, 255.0f)};
        }
      }
      particle.update(delta_ms);
    }
  }

  void render(sf::RenderWindow &window) {
    vertex_array.clear();
    for (const auto &particle : particles) {
      sf::Vector2f top_right{particle.position};
      sf::Vector2f top_left{particle.position.x + particle.size.x,
                            particle.position.y};
      sf::Vector2f bottom_left{particle.position.x,
                               particle.position.y + particle.size.y};

      sf::Vector2f bottom_right{particle.position.x + particle.size.x,
                                particle.position.y + particle.size.y};

      const sf::Color vertex_color{
          color.r, color.g, color.b,
          static_cast<uint8_t>(
              std::max(0.0f, std::min(particle.lifespan, 255.0f)))};

      vertex_array.emplace_back(bottom_right, vertex_color,
                                sf::Vector2f{0.0f, 0.0f});
      vertex_array.emplace_back(top_left, vertex_color,
                                sf::Vector2f{0.0f, 0.0f});
      vertex_array.emplace_back(top_right, vertex_color,
                                sf::Vector2f{0.0f, 0.0f});
      vertex_array.emplace_back(top_right, vertex_color,
                                sf::Vector2f{0.0f, 0.0f});
      vertex_array.emplace_back(bottom_right, vertex_color,
                                sf::Vector2f{0.0f, 0.0f});
      vertex_array.emplace_back(bottom_left, vertex_color,
                                sf::Vector2f{0.0f, 0.0f});
    }

    window.draw(vertex_array.data(), vertex_array.size(), sf::Triangles);
  }
};

int main() {
  sf::ContextSettings window_settings{};
  window_settings.antialiasingLevel = 8;

  sf::RenderWindow window{
      sf::VideoMode{config::window_width, config::window_height},
      std::string{config::window_title}, sf::Style::Default, window_settings};

  window.setVerticalSyncEnabled(true);

  sf::View render_view{
      sf::FloatRect{0.0f, 0.0f, static_cast<float>(config::render_width),
                    static_cast<float>(config::render_height)}};

  sf::View window_view{
      sf::FloatRect{0.0f, 0.0f, static_cast<float>(config::window_width),
                    static_cast<float>(config::window_height)}};

  sf::RenderTexture render_texture{};
  if (!render_texture.create(config::render_width, config::render_height)) {
    std::cerr << "Failed to create a render texture." << std::endl;
    return EXIT_FAILURE;
  }

  sf::Font font{};
  sf::Text text{};

  if (!font.loadFromFile("m6x11plus.ttf")) {
    std::cerr << "Failed to create a render texture." << std::endl;
    return EXIT_FAILURE;
  }

  text.setFont(font);
  text.setCharacterSize(512);
  text.setFillColor(sf::Color{0x1F1F1FFF});
  text.setString("0");
  text.setPosition(
      sf::Vector2f{static_cast<float>(config::window_width / 2.0f -
                                      text.getString().getSize() * 64),
                   static_cast<float>(config::render_height / 2.0f - 64)});

  float score{};

  auto start{std::chrono::high_resolution_clock::now()};
  bool top{true}, game_start{}, space_held{};
  float elapsed_ms{};

  Player ship{config::render_width / 2 - config::player_width / 2,
              config::render_height / 2 - config::player_height / 2, 0,
              config::player_velocity};
  sf::RectangleShape ship_render_rect{
      sf::Vector2f{config::player_width, config::player_height}};
  ship.texture.loadFromFile("ship.png");
  ship_render_rect.setFillColor(sf::Color{config::color_palette::blue});

  sf::RectangleShape ceiling{sf::Vector2f{
      static_cast<float>(config::render_width * 4), config::gap * 10}};
  ceiling.setPosition(
      sf::Vector2f{0.0f - config::render_width, 0 - config::gap * 9});
  ceiling.setFillColor(sf::Color{config::color_palette::orange});

  sf::RectangleShape floor{sf::Vector2f{
      static_cast<float>(config::render_width * 4), config::gap * 10}};
  floor.setPosition(sf::Vector2f{0.0f - config::render_width,
                                 config::render_height - config::gap});
  floor.setFillColor(sf::Color{config::color_palette::orange});

  std::vector<sf::Vertex> spikes_vertex_buffer{};
  std::vector<sf::Vertex> trail{};
  float theta{};

  ParticleSystem thruster = ParticleSystem::create(
      sf::Vector2f{ship.x, ship.y}, 500, sf::Color{0x7DC7FFFF}, true);

  while (window.isOpen()) {
    if (score >= 0) {
      text.setString(std::to_string(static_cast<uint32_t>(score)));
      text.setPosition(
          sf::Vector2f{static_cast<float>(config::window_width / 2.0f -
                                          text.getString().getSize() * 64),
                       static_cast<float>(config::render_height / 2.0f - 64)});
    }

    spikes_vertex_buffer.clear();
    sf::Event event{};
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed)
        window.close();
      if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && !space_held) {
        if (!ship.is_dead())
          game_start = true;
        ship.delta_y = -ship.delta_y;
        space_held = true;
      }
      if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && space_held) {
        space_held = false;
      }
    }

    auto current{std::chrono::high_resolution_clock::now()};
    auto delta{current - start};

    float delta_ms{static_cast<float>(
        std::chrono::duration_cast<std::chrono::milliseconds>(delta).count())};

    elapsed_ms += delta_ms;

    if (game_start) {
      ship.update(delta_ms);
      config::spike_velocity = 2e-1f;
      game_start = !ship.is_dead();
    } else {
      config::spike_velocity =
          std::max(config::spike_velocity - config::spike_acceleration, 0.0f);
    }

    if (config::spike_velocity) {
      theta += 1e-2f * delta_ms;
      for (auto &spike : spikes) {
        spike.v1.position.x -= (config::spike_velocity * delta_ms);
        spike.v2.position.x -= (config::spike_velocity * delta_ms);
        spike.v3.position.x -= (config::spike_velocity * delta_ms);

        auto sign = [=](sf::Vector2f p1, sf::Vector2f p2,
                        sf::Vector2f p3) -> float {
          return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
        };

        auto point_in_triangle = [&](sf::Vector2f point,
                                     const Spike &triangle) -> bool {
          float d1{}, d2{}, d3{};
          bool has_neg{}, has_pos{};
          d1 = sign(point, triangle.v1.position, triangle.v2.position);
          d2 = sign(point, triangle.v2.position, triangle.v3.position);
          d3 = sign(point, triangle.v3.position, triangle.v1.position);
          has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
          has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
          return !(has_neg && has_pos);
        };

        if (point_in_triangle(
                sf::Vector2f(ship.x + config::player_width, ship.y), spike) ||
            point_in_triangle(sf::Vector2f(ship.x + config::player_width,
                                           ship.y + config::player_height),
                              spike) ||
            point_in_triangle(sf::Vector2f(ship.x + config::player_width,
                                           ship.y + config::player_height),
                              spike)) {
          ship.is_dead(true);
        }
      }

      if (static_cast<int32_t>(elapsed_ms) > 700) {
        float width{rng::f32(30.0f, 60.0f)};
        float height{rng::f32(50.0f, 80.0f)};
        generate_spike(config::render_width + 60, width, height, top);
        generate_spike(config::render_width + 60 + 6 * config::gap, width,
                       height, !top);
        elapsed_ms = 0;
      }
    }

    for (auto it = spikes.begin(); it != spikes.end();) {
      if (!ship.is_dead()) {
        if (it->v2.position.x < ship.x && !it->passed) {
          score++;
          it->passed = true;
        }
      }
      if (it->v3.position.x < static_cast<int32_t>(-config::render_width)) {
        spikes.erase(it);
      } else {
        spikes_vertex_buffer.push_back(it->v1);
        spikes_vertex_buffer.push_back(it->v2);
        spikes_vertex_buffer.push_back(it->v3);
        it++;
      }
    }

    if (!ship.is_dead()) {
      ship_render_rect.setPosition(ship.x, ship.y);
      thruster.update(delta_ms);
      thruster.set_position(sf::Vector2f{ship.x, ship.y});
    } else {
      static float radius{30.0f};
      static float random_angle{rng::f32(0.0f, 360.0f)};
      static sf::Vector2f original_center{render_view.getCenter()};
      static sf::Vector2f offset{
          static_cast<float>(std::sin(random_angle) * radius),
          static_cast<float>(std::cos(random_angle) * radius)};
      if (radius > 1.0f) {
        radius *= 0.8f;
        random_angle = rng::f32(0.0f, 360.0f);
        random_angle = degrees_to_radians(random_angle);
        offset = sf::Vector2{
            static_cast<float>(std::sin(random_angle) * radius),
            static_cast<float>(std::cos(random_angle) * radius / 4)};
        render_view.setCenter(render_view.getCenter() + offset);
      } else {
        if (render_view.getCenter().x < original_center.x)
          render_view.move(0.5f, 0.0f);
        if (render_view.getCenter().x > original_center.x)
          render_view.move(-0.5f, 0.0f);
        if (render_view.getCenter().y < original_center.y)
          render_view.move(0.0f, 1.0f);
        if (render_view.getCenter().y > original_center.y)
          render_view.move(0.0f, -1.0f);
      }
    }

    window.clear(sf::Color{config::color_palette::gray});

    window.setView(window_view);
    window.draw(text);

    window.setView(render_view);
    window.draw(ceiling);
    window.draw(floor);
    window.draw(spikes_vertex_buffer.data(), spikes_vertex_buffer.size(),
                sf::Triangles);
    render_view.setRotation(theta);

    if (!ship.is_dead()) {
      window.draw(ship_render_rect);
    }

    window.display();

    start = current;
  }

  return EXIT_SUCCESS;
}
