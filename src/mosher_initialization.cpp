#include "mosher.hpp"
#include "moshpit.cpp"
#include "vector.hpp"
#include <algorithm>
#include <csignal>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <cmath>

//active - доля активных челов дает булев вектор активности для инициализации
std::vector<bool> generate_mask_for_mosher_activity(int32_t number_of_people, float active) {
  std::vector<bool> activity(number_of_people, false);
  auto active_people = static_cast<int32_t>(number_of_people * active);
  std::fill(activity.begin(), activity.begin() + active_people, true);
  return activity;
}

//Далее функция которая собирает людей в вектор и шафлит их позиция пока не инитится
//В начале сделаем просто что все люди одинаковые 
std::vector<Mosher> initialize_people(int32_t number_of_people, float range, float mass, float radius, float active_velocity, const std::vector<bool>& mask) {
  std::vector<Mosher> people;

  std::random_device rand;
  std::mt19937 gen(rand());
  std::uniform_real_distribution<float> angle_dist(0.0f, 2.0f * M_PI);

  for (int32_t i = 0; i < number_of_people; i++) {
    Status stat = PASSIVE;
    float velocity = 0;
    Vector start_velocity = {0, 0};
    if (mask[i]) {
      stat = ACTIVE;
      velocity = active_velocity;
      float angle = angle_dist(gen);
      start_velocity.x = active_velocity * std::cos(angle);
      start_velocity.y = active_velocity * std::sin(angle);
    }
    people.emplace_back(range, mass, radius, stat, velocity, i);
    people.back().current_velocity = start_velocity;
  }
  //И шаффлим
  std::shuffle(people.begin(), people.end(), gen);
  return people;
}

//Размер плотность упаковки будем определять размером коробки и количеством людей 
//Далее функция, инициализирующая координаты, работаем все еще в предположении одинаковых радиусов
void initialize_positions(float box_size, std::vector<Mosher>& people) {
  auto number_of_people = people.size();
  float square = 0.0f;
  float packing_fraction = 0;
  
  for (auto& mosher : people) {
    square += 3.14 * mosher.radius * mosher.radius;
  }
  packing_fraction = square / (box_size * box_size);
  if (packing_fraction >= 1) {
    throw std::runtime_error("Площадь людей превысила размер площадки");
  }

  int32_t cells_per_side = static_cast<int32_t>(std::ceil(std::sqrt(number_of_people)));
  float step = box_size / cells_per_side;
  size_t spawned = 0;
  for (size_t i = 0; i < cells_per_side && spawned < number_of_people; i++) {
    for (size_t j = 0; j < cells_per_side && spawned < number_of_people; j++) {
      float x = i * step + step / 2.0f;
      float y = j * step + step / 2.0f;
      people[spawned].position.x = x;
      people[spawned].position.y = y;
      spawned++;
    }
  }
}

void initialize_positions_hex(float box_size, std::vector<Mosher>& people) {
  auto number_of_people = people.size();
  float square = 0.0f;
  float packing_fraction = 0;
  
  for (auto& mosher : people) {
    square += 3.14 * mosher.radius * mosher.radius;
  }
  packing_fraction = square / (box_size * box_size);
  std::cout << "Плотность упаковки" << packing_fraction << '\n';
  if (packing_fraction >= 1) {
    throw std::runtime_error("Площадь людей превысила размер площадки");
  }
  float diameter = people[0].radius * 2.0f;
  float row_height = diameter * 0.866f;
  
  int32_t cols = static_cast<int32_t>(box_size / diameter);
  int32_t rows = static_cast<int32_t>(box_size / row_height);
    
  size_t spawned = 0;
  for (int32_t i = 0; i < rows && spawned < people.size(); i++) {
    for (int32_t j = 0; j < cols && spawned < people.size(); j++) {

      float offset_x = (i % 2 == 0) ? 0.0f : people[0].radius;
      float x = offset_x + j * diameter + people[0].radius;
      float y = i * row_height + people[0].radius;
            
      if (x < box_size && y < box_size) {
        people[spawned].position.x = x;
        people[spawned].position.y = y;
        spawned++;
      }
    }
  }
}

std::vector<Mosher> initialize_authors_moshpit(
  int32_t number_of_people, 
  float box_size, 
  float active_fraction, 
  float range, 
  float mass, 
  float radius, 
  float active_velocity
) {
  std::vector<Mosher> people;
  people.reserve(number_of_people);
  auto fraction = number_of_people * 3.14 * radius * radius / (box_size * box_size);
  std::cout << "Плотность упаковки: " << fraction;

  std::random_device rd;
  std::mt19937 gen(rd());
  // Распределение координат по всей площади площадки
  std::uniform_real_distribution<float> pos_dist(0.0f, box_size);
  // Распределение направлений начальной скорости
  std::uniform_real_distribution<float> angle_dist(0.0f, 2.0f * 3.1415926535f);

  float center_x = box_size / 2.0f;
  float center_y = box_size / 2.0f;

  // Считаем радиус активной зоны в центре.
  // Формула: Площадь_круга = active_fraction * Общая_площадь_бокса
  // pi * R^2 = active_fraction * L^2
  float active_zone_radius = std::sqrt((active_fraction * box_size * box_size) / 3.1415926535f);

  for (int32_t i = 0; i < number_of_people; ++i) {
    float x = pos_dist(gen);
    float y = pos_dist(gen);

    float dx = x - center_x;
    float dy = y - center_y;
    float dist_to_center = std::sqrt(dx * dx + dy * dy);

    Status stat = PASSIVE;
    float velocity = 0.0f;
    Vector start_velocity = {0.0f, 0.0f};

    // Если сгенерированная координата попала в центральный круг - делаем агента активным
    if (dist_to_center < active_zone_radius) {
      stat = ACTIVE;
      velocity = active_velocity;
      float angle = angle_dist(gen);
      start_velocity.x = active_velocity * std::cos(angle);
      start_velocity.y = active_velocity * std::sin(angle);
    }

    // Вызываем твой конструктор (порядок аргументов из твоего кода)
    people.emplace_back(range, mass, radius, stat, velocity, i);
    
    // Перезаписываем позицию и скорость
    people.back().position = {x, y};
    people.back().current_velocity = start_velocity;
  }

  return people;
}