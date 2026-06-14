#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "src/mosher.hpp"
#include "src/moshpit.cpp"
#include "src/vector.hpp"

extern std::vector<bool>
generate_mask_for_mosher_activity(int32_t number_of_people, float active);
extern std::vector<Mosher> initialize_people(int32_t number_of_people,
                                             float range, float mass,
                                             float radius,
                                             float active_velocity,
                                             const std::vector<bool> &mask);
extern void initialize_positions(float box_size, std::vector<Mosher> &people);

#pragma pack(push, 1)
struct AgentData {
  float x;
  float y;
  char type;
};
#pragma pack(pop)

int main() {
  int32_t N = 500;
  float L = 20.0f;
  float dt = 0.05f;
  float active_fraction = 0.3f;
  float fluct = 1.5f;
  float flock = 0.5f;
  int num_frames = 100; // Ровно 100 файлов

  float range_of_view = 4.0f;
  float mass = 1.0f;
  float radius = 1.0f;
  float goal_vel = 1.0f;

  SystemConstants consts(25.0f, 1.0f, range_of_view, mass, radius, goal_vel);

  std::vector<bool> mask =
      generate_mask_for_mosher_activity(N, active_fraction);
  std::vector<Mosher> people =
      initialize_people(N, range_of_view, mass, radius, goal_vel, mask);
  initialize_positions(L, people);
  float packing_fraction = (N * 3.1415f * radius * radius) / (L * L);

  MoshPit pit(people, dt, L, L, packing_fraction, N, fluct, flock,
              range_of_view, consts);

  std::cout << "Запуск симуляции. Будет создано " << num_frames
            << " бинарных файлов..." << std::endl;

  for (int frame = 0; frame < num_frames; ++frame) {
    double current_time = frame * dt;

    // Формируем имя файла с ТЕКУЩИМ номером кадра (frame)
    std::string filename =
        "sim_N" + std::to_string(N) + "_L" + std::to_string(L).substr(0, 4) +
        "_iter" + std::to_string(frame) + // меняется от 0 до 99
        "_alpha" + std::to_string(flock).substr(0, 3) + "_sigma" +
        std::to_string(fluct).substr(0, 3) + "_seed42.bin";

    std::ofstream outfile(filename, std::ios::binary);

    outfile.write(reinterpret_cast<const char *>(&current_time),
                  sizeof(double));

    std::vector<AgentData> out_data(N);
    for (int i = 0; i < N; ++i) {
      out_data[i].x = pit.people[i].position.x;
      out_data[i].y = pit.people[i].position.y;
      out_data[i].type = (pit.people[i].state == Status::ACTIVE) ? 'a' : 'p';
    }
    outfile.write(reinterpret_cast<const char *>(out_data.data()),
                  N * sizeof(AgentData));

    outfile.close(); // Закрываем файл после записи 1 кадра!

    // Делаем физический шаг
    pit.make_step_n_iterations(1);
  }

  std::cout << "Готово! 100 бинарников успешно сохранены." << std::endl;
  return 0;
}