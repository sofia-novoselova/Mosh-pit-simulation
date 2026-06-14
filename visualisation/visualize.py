import os
import re
import struct
import glob
import numpy as np
import matplotlib.pyplot as plt


def parse_filename(fpath):
    """Получает путь и достает параметры из имени"""
    fname = os.path.basename(fpath)
    patt = r'sim_N(\d+)_L([0-9.]+)_iter(\d+)_alpha([0-9.]+)_sigma([0-9.]+)_seed(\d+)\.bin'
    match = re.match(patt, fname)

    if not match:
        raise ValueError(f"Имя файла '{fname}' не соответствует формату.")

    params = {
        'N': int(match.group(1)),
        'L': float(match.group(2)),
        'it': int(match.group(3)),
        'alpha': float(match.group(4)),
        'sigma': float(match.group(5)),
        'seed': int(match.group(6))
    }
    return params


def read_data(fpath, N):
    """Читает бинарный файл и возвращает данные"""
    agent_dtype = np.dtype([('x', '<f4'), ('y', '<f4'), ('type', 'S1')])
    times = []
    positions_list = []
    types_list = []

    frame_agents_bytes_size = N * agent_dtype.itemsize

    with open(fpath, 'rb') as f:
        while True:
            time_bytes = f.read(8)
            if not time_bytes or len(time_bytes) < 8:
                break

            current_time = struct.unpack('<d', time_bytes)[0]
            agents_bytes = f.read(frame_agents_bytes_size)
            if len(agents_bytes) < frame_agents_bytes_size:
                break

            agents_data = np.frombuffer(agents_bytes, dtype=agent_dtype)
            times.append(current_time)
            positions = np.column_stack((agents_data['x'], agents_data['y']))
            positions_list.append(positions)
            types_list.append(agents_data['type'])

    return times, positions_list, types_list


def save_all_frames_to_images(times, positions_list, types_list, params, filepath):
    """Рисует и сохраняет каждый кадр как отдельный PNG файл."""

    # Создаем папку для картинок на основе имени бинарника
    # Например: если файл sim_N400...bin, папка будет frames_sim_N400...
    base_name = os.path.splitext(os.path.basename(filepath))[0]
    out_dir = os.path.join(os.path.dirname(filepath), f"frames_{base_name}")
    os.makedirs(out_dir, exist_ok=True)

    fig, ax = plt.subplots(figsize=(6, 6))

    L = params['L']
    ax.set_xlim(0, L)
    ax.set_ylim(0, L)
    ax.set_aspect('equal')
    ax.set_xticks([])
    ax.set_yticks([])

    scatter = ax.scatter([], [], s=20)
    info_text = ax.text(0.02, 0.98, '', transform=ax.transAxes, va='top',
                        bbox=dict(boxstyle='round', facecolor='white', alpha=0.8))

    print(f"Сохраняем картинки в папку: {out_dir} ...")

    # Идем по каждому кадру
    for i in range(len(times)):
        positions = positions_list[i]
        agent_types = types_list[i]
        current_time = times[i]

        scatter.set_offsets(positions)

        colors = np.where(agent_types == b'a', 'red', 'blue')
        scatter.set_color(colors)

        text_str = (f"Time: {current_time:.2f} s\n"
                    f"N={params['N']}\n"
                    f"Alpha={params['alpha']}\n"
                    f"Sigma={params['sigma']}\n"
                    f"Iter: {params['it']}")
        info_text.set_text(text_str)

        # Сохраняем (например: frame_0000.png, frame_0001.png ...)
        frame_filename = os.path.join(out_dir, f"frame_{i:04d}.png")
        # dpi=100 для стандартного качества. Для 4к ставьте dpi=300
        fig.savefig(frame_filename, dpi=100)

        # Выводим прогресс
        if (i + 1) % 20 == 0:
            print(f"  Отрендерено {i + 1} / {len(times)} кадров...")

    plt.close(fig)
    print("Успешно завершено!\n")


if __name__ == "__main__":
    target_folder = "../build/"

    bin_files = glob.glob(os.path.join(target_folder, "*.bin"))

    if not bin_files:
        print(f"Файлы .bin не найдены в папке {target_folder}!")

    for filepath in bin_files:
        print(f"--- Обрабатываю файл: {os.path.basename(filepath)} ---")
        try:
            sim_params = parse_filename(filepath)
            t, pos, types = read_data(filepath, sim_params['N'])

            # Вместо гифки вызываем нарезку на картинки
            save_all_frames_to_images(t, pos, types, sim_params, filepath)

        except Exception as e:
            print(f"Ошибка при обработке {filepath}: {e}")
