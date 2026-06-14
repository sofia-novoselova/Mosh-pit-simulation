#pragma once
#include <cstddef>
#include <iostream>
#include <vector>
#include "vector.hpp"

enum Status {
  ACTIVE,
  PASSIVE
};

//position - буквально где он, со скоростями очев(goal - целевая)
//range - расстояние на котором коллективизация работает, масса очев, радиус - человека как шарика
struct Mosher {
 public:
  Vector position;
  Vector current_velocity;
  Vector total_strenth = {0, 0};

  float range;
  float mass;
  float radius;
  Status state = Status::PASSIVE;
  float goal_velocity_abs;
  int32_t person_id;

  Mosher() = default;

  Mosher(float rang, float mas, float rad, Status stat, float goal_vel, int32_t i);

  Mosher(float rang, float mas, float rad, Status stat, float goal_vel, Vector pos, Vector cur_vel, int32_t i);
  //Идея этой хуйни всех даунов с одного вектора инициализировать
  Mosher(const std::vector<float>& params, Status stat, float goal_vel, Vector pos, Vector cur_vel, int32_t i);
  Mosher(const std::vector<float>& params, Status stat, float goal_vel, int32_t i);
};


