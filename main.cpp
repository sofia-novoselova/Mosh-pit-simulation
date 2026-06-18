#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "src/mosher.hpp"
#include "src/moshpit.cpp"
#include "src/vector.hpp"

extern std::vector<Mosher> initialize_authors_moshpit(
  int32_t number_of_people, float box_size, float active_fraction, 
  float range, float mass, float radius, float active_velocity);

#pragma pack(push, 1)
struct AgentData {
  float x;
  float y;
  float r;
  float vx;
  float vy;
  char type;
};
#pragma pack(pop)

int main() {
  int32_t N = 380;
  float L = 35.0f;
  float dt = 0.1f;
  float active_fraction = 0.3f;
  float fluct = 0.09f;
  float flock = 0.01f;
  int num_frames = 500; // Ровно 100 файлов

  float range_of_view = 4.0f;
  float mass = 1.0f;
  float radius = 1.0f;
  float goal_vel = 1.0f;

  SystemConstants consts(25.0f, 1.0f, range_of_view, mass, radius, goal_vel);

  std::vector<Mosher> people = initialize_authors_moshpit(
    N, L, active_fraction, range_of_view, mass, radius, goal_vel
  );

  float packing_fraction = (N * 3.1415f * radius * radius) / (L * L);

  MoshPit pit(people, dt, L, L, packing_fraction, N, fluct, flock,
              range_of_view, consts);

  std::cout << "Запуск симуляции. Будет создано " << num_frames
            << " бинарных файлов..." << std::endl;
  // pit.make_step_n_iterations_with_potential(0);
  // pit.make_step_n_iterations(500);
  for (int frame = 0; frame < num_frames; ++frame) {
    double current_time = frame * dt;

    // Формируем имя файла с ТЕКУЩИМ номером кадра (frame)
    std::string filename =
        "sim_N" + std::to_string(N) + "_L" + std::to_string(L).substr(0, 4) +
        "_iter" + std::to_string(frame) + // меняется от 0 до 99
        "_alpha" + std::to_string(flock).substr(0, 5) + "_sigma" +
        std::to_string(fluct).substr(0, 5) + "_seed42.bin";

    std::ofstream outfile(filename, std::ios::binary);

    outfile.write(reinterpret_cast<const char *>(&current_time),
                  sizeof(double));

    std::vector<AgentData> out_data(N);
    for (int i = 0; i < N; ++i) {
      out_data[i].x = pit.people[i].position.x;
      out_data[i].y = pit.people[i].position.y;
      out_data[i].r = pit.people[i].radius;
      out_data[i].vx = pit.people[i].current_velocity.x;
      out_data[i].vy = pit.people[i].current_velocity.y;
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