#include "mosher.hpp"
#include "vector.hpp"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

//TODO
//В эту структуру кидаем все системные констаны (жесткости людей, коэффициент коллективизации и тд)
//Короче все что не поле класса для мошеров глобально
struct SystemConstants {
  float repulsion_strength;
  float propulsion_strength;
  float people_range;
  float people_mass;
  float people_radius;
  float people_goal_velocity;

  SystemConstants() = default;
  SystemConstants(float rs, float ps, float pra, float pm, float prad, float pgv) {
    repulsion_strength = rs;
    propulsion_strength = ps;
    people_range = pra;
    people_mass = pm;
    people_radius = prad;
    people_goal_velocity = pgv; 
  }
};

struct MoshPit {
  std::mt19937 generator;
  std::normal_distribution<float> dist{0.0f, 1.0f};
  std::vector<Mosher> people;
  std::vector<std::vector<Mosher*>> net;
  SystemConstants constants;
  float time_step;
  float length;
  float width;

  float packing_fraction;
  float number_of_people;

  float fluct_strength;
  float flocking_strength;

  float range_of_view;

  int32_t cell_per_side;
  int32_t total_cells;

  MoshPit() = default;
  MoshPit(std::vector<Mosher> peop, float step, float leng, float wid, float pf, float nop, float fluct, float flock, float rov, SystemConstants consts) : generator(std::random_device{}()) {
    people = peop;
    time_step = step;

    length = leng;
    width = wid;

    packing_fraction = pf;
    number_of_people = nop;

    fluct_strength = fluct;
    flocking_strength = flock;
    range_of_view = rov;

    constants = consts;
    cell_per_side = static_cast<int32_t>(length / range_of_view);
    total_cells = static_cast<int32_t>(length * width / (range_of_view * range_of_view));
    net = std::vector<std::vector<Mosher*>> (total_cells);
  }
  //Далее функция для определения, к какой ячейке сетки относится чел
  //Пока тупо для удобства будем считать длину и ширину одинаковыми, причем кратными range
  int32_t find_cell(const Vector& current_position) const {
    auto x = current_position.x;
    auto y = current_position.y;

    int32_t x_cells = length * range_of_view;
    int32_t x_cell = static_cast<int32_t>(x / range_of_view);
    int32_t y_cell = static_cast<int32_t>(y / range_of_view);

    if (x_cell >= cell_per_side) x_cell = cell_per_side - 1;
    if (y_cell >= cell_per_side) y_cell = cell_per_side - 1;

    return y_cell * cell_per_side + x_cell;
  }

  //Функция ищущая индексы ячеек потенциальных соседей
  void find_neigbour_ids(int32_t cell, std::vector<int32_t>& ids) const {
    ids.clear();
    int32_t cell_x = cell % cell_per_side;
    int32_t cell_y = cell / cell_per_side;

    for (int32_t dy = -1; dy < 2; dy++) {
      for (int32_t dx = -1; dx < 2; dx++) {
        int32_t neighb_x = cell_x + dx;
        int32_t neighb_y = cell_y + dy;
        if (neighb_x >=0 && neighb_x < cell_per_side && neighb_y >= 0 && neighb_y < cell_per_side) {
          ids.push_back(neighb_y * cell_per_side + neighb_x);
        }
      }
    }
  }
  //Далее логики для поиска соседей для флокинга и репульсионной силы
  //Здесь выстраиваем вектор принадлежностей мошеров ячейкам
  void make_moshers_net() {
    for (auto& cell : net) {
      cell.clear();
    }
    for (auto& mosher : people) {
      auto cell = find_cell(mosher.position);
      net[cell].push_back(&mosher);
    }
  }

  void find_neigbs_for_flock_and_rep(const Mosher& person, const std::vector<std::vector<Mosher*>>& net, const std::vector<int32_t>& neighbours_id, std::vector<Mosher*>& repulsion_neighbs, std::vector<Mosher*>& flock_neighbs) {
    repulsion_neighbs.clear();
    flock_neighbs.clear();
    for (auto id : neighbours_id) {
      for (auto& mosher : net[id]) {
        if (mosher->person_id == person.person_id) {
          continue;
        }
        auto delta_vector = mosher->position - person.position;
        auto delta_position = delta_vector.get_squared_magnitude();
        if (delta_position < range_of_view * range_of_view) {
          flock_neighbs.push_back(mosher);
        }
        if (delta_position < 4 * (mosher->radius + person.radius) * (mosher->radius + person.radius)) {
          repulsion_neighbs.push_back(mosher);
        }
      }
    }
  }
  //Дальше функции для счета сил
  //Сначала репульсионная сила
  Vector get_repulsion_force(const Mosher& person, std::vector<Mosher*>& repulsion_neighbs) {
    Vector full_force = {0, 0};
    for (auto& mosher : repulsion_neighbs) {
      auto diff = mosher->position - person.position;
      auto coefficent = (1 - diff.get_magnitude() / (person.radius + mosher->radius));
      auto repulsion_force = constants.repulsion_strength * coefficent * std::sqrt(coefficent) * (diff / diff.get_magnitude());
      full_force += repulsion_force;
    }
    return full_force;
  }

  //Пропульсионная
  Vector get_propulsion_force(const Mosher& person) {
    auto velocity_diff = (person.goal_velocity_abs - person.current_velocity.get_magnitude());
    return constants.propulsion_strength * velocity_diff * (person.current_velocity / person.current_velocity.get_magnitude());
  }

  //Далее сила возникающая возникающая в результате коллективизации
  Vector get_flocking_force(const Mosher& person, std::vector<Mosher*>& flock_neighbs) {
    Vector flock_vector = {0, 0};
    for (auto& mosher : flock_neighbs) {
      flock_vector += mosher->current_velocity;
    }
    if (flock_vector.x == 0 && flock_vector.y == 0) {
      return flock_vector;
    }
    auto flock_force = flocking_strength * flock_vector / flock_vector.get_magnitude();
    return flock_force;
  }

  //TODO ввести по аналогии с репульсионной силой взаимодействие со стенами
  Vector get_wall_force(const Mosher& percon) {
    return {0, 0};
  }

  Vector get_total_determ_force(const Mosher& person, std::vector<std::vector<Mosher*>>& net, std::vector<int32_t>& neighbours_id, std::vector<Mosher*>& flock_neighbs, std::vector<Mosher*>& repulsion_neighbs) {
    auto cell = find_cell(person.position);
    find_neigbour_ids(cell, neighbours_id);
    find_neigbs_for_flock_and_rep(person, net, neighbours_id, repulsion_neighbs, flock_neighbs);
    auto repulsion_force = get_repulsion_force(person, repulsion_neighbs);
    auto propulsion_force = get_propulsion_force(person);
    
    Vector flocking_force = {0, 0};
    if (person.state == Status::ACTIVE) {
      flocking_force = get_flocking_force(person, flock_neighbs);
    }
    return propulsion_force + flocking_force - repulsion_force;
  }

  Vector get_stochastic_step(const Mosher& person) {
    auto sqrt_dt = std::sqrt(time_step);
    float rand_x = dist(generator);
    float rand_y = dist(generator);
    Vector rand_vec = {rand_x, rand_y};
    auto stochastic_step = fluct_strength * sqrt_dt / person.mass * rand_vec;
    return stochastic_step;
  }
  //Для начала пересчитываем все детерминированные силы 
  void set_determ_force(std::vector<std::vector<Mosher*>>& net, std::vector<int32_t>& neighbours_id, std::vector<Mosher*>& flock_neighbs, std::vector<Mosher*>& repulsion_neighbs) {
    for (auto& mosher : people) {
      mosher.total_strenth = get_total_determ_force(mosher, net, neighbours_id, flock_neighbs, repulsion_neighbs);
    }
  }

  void make_step_one_agent_active(Mosher& person, std::vector<std::vector<Mosher*>>& net, std::vector<int32_t>& neighbours_id, std::vector<Mosher*>& flock_neighbs, std::vector<Mosher*>& repulsion_neighbs) {
    auto determ_force = person.total_strenth;
    auto stochastic_step = get_stochastic_step(person);
    person.current_velocity = person.current_velocity + determ_force * time_step / person.mass + stochastic_step;
    person.position = person.position + person.current_velocity * time_step;
  }

  void make_step_one_agent_passive(Mosher& person, std::vector<std::vector<Mosher*>>& net, std::vector<int32_t>& neighbours_id, std::vector<Mosher*>& flock_neighbs, std::vector<Mosher*>& repulsion_neighbs) {
    auto determ_force = person.total_strenth;
    person.current_velocity = person.current_velocity + determ_force * time_step / person.mass;
    person.position = person.position + person.current_velocity * time_step;
  }

  void make_step_n_iterations(size_t n_iterations) {
    std::vector<int32_t> neighbours_id;
    std::vector<Mosher*> flock_neighbs;
    std::vector<Mosher*> repulsion_neighbs;
    for (size_t i = 0; i < n_iterations; i++)  {
      auto net = make_moshers_net();
      set_determ_force(net, neighbours_id, flock_neighbs, repulsion_neighbs);
      for (auto& mosher : people) {
        if (mosher.state == Status::PASSIVE) {
          make_step_one_agent_passive(mosher, net, neighbours_id,flock_neighbs, repulsion_neighbs);
        } else {
          make_step_one_agent_active(mosher, net, neighbours_id,flock_neighbs, repulsion_neighbs);
        }
      }
    }
  }
};