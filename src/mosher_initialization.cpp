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