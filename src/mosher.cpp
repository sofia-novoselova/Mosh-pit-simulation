#include "mosher.hpp"

Mosher::Mosher(float rang, float mas, float rad, Status stat, float goal_vel, int32_t i) : range(rang), mass(mas), radius(rad), state(stat), goal_velocity_abs(goal_vel), person_id(i) {
  position = {0, 0};
  current_velocity = {0, 0};
}

Mosher::Mosher(float rang, float mas, float rad, Status stat, float goal_vel, Vector pos, Vector cur_vel, int32_t i) : range(rang), mass(mas), radius(rad), state(stat), goal_velocity_abs(goal_vel), position(pos), current_velocity(cur_vel), person_id(i) {
}

Mosher::Mosher(const std::vector<float>& params, Status stat, float goal_vel, Vector pos, Vector cur_vel, int32_t i) {
  range = params[0];
  mass = params[1];
  radius = params[2];
  state = stat;
  goal_velocity_abs = goal_vel;
  current_velocity = cur_vel;
  position = pos;
  person_id = i;
};

Mosher::Mosher(const std::vector<float>& params, Status stat, float goal_vel, int32_t i) {
  range = params[0];
  mass = params[1];
  radius = params[2];
  state = stat;
  goal_velocity_abs = goal_vel;
  current_velocity = {0, 0};
  position = {0, 0};
  person_id = i;
};