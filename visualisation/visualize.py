import os
import re
import struct
import glob
import io
import numpy as np
import matplotlib.pyplot as plt
from PIL import Image
from collections import defaultdict


def parse_filename(fpath):
    fname = os.path.basename(fpath)
    patt = r'sim_N(\d+)_L([0-9.]+)_iter(\d+)_alpha([0-9.]+)_sigma([0-9.]+)_seed(\d+)\.bin'
    match = re.match(patt, fname)
    if not match:
        return None

    return {
        'N': int(match.group(1)),
        'L': float(match.group(2)),
        'it': int(match.group(3)),
        'alpha': float(match.group(4)),
        'sigma': float(match.group(5)),
        'seed': int(match.group(6)),
        # base_name группирует все 100 файлов одной симуляции
        'base_name': f"sim_N{match.group(1)}_L{match.group(2)}_alpha{match.group(4)}_sigma{match.group(5)}_seed{match.group(6)}"
    }


def read_single_frame(fpath, N):
    agent_dtype = np.dtype([('x', '<f4'), ('y', '<f4'), ('type', 'S1')])
    with open(fpath, 'rb') as f:
        t_bytes = f.read(8)
        if len(t_bytes) < 8:
            return None, None, None

        current_time = struct.unpack('<d', t_bytes)[0]
        ag_bytes = f.read(N * agent_dtype.itemsize)

        agents_data = np.frombuffer(ag_bytes, dtype=agent_dtype)
        positions = np.column_stack((agents_data['x'], agents_data['y']))
        return current_time, positions, agents_data['type']


if __name__ == "__main__":
    print("Начали")
    target_folder = "/home/redh9ad/people_modeling/build/"
    print(f"Ищу файлы в папке: {os.path.abspath(target_folder)}")
    
    bin_files = glob.glob(os.path.join(target_folder, "*.bin"))
    print(f"Найдено файлов .bin: {len(bin_files)}")

    # 1. Группируем файлы
    simulations = defaultdict(list)
    for f in bin_files:
        p = parse_filename(f)
        if p:
            simulations[p['base_name']].append((p['it'], f, p))

    # 2. Рендерим каждую симуляцию
    for base_name, files in simulations.items():
        # СОРТИРУЕМ строго по номеру iter (чтобы шли 0, 1, 2... 99)
        files.sort(key=lambda x: x[0])
        main_params = files[0][2]
        L = main_params['L']

        print(f"Сборка: {base_name} ({len(files)} итераций/кадров)")

        frames_for_gif = []

        # Настраиваем полотно ОДИН раз
        fig, ax = plt.subplots(figsize=(6, 6))
        ax.set_xlim(0, L)
        ax.set_ylim(0, L)
        ax.set_aspect('equal')
        ax.set_xticks([])
        ax.set_yticks([])

        scatter = ax.scatter([], [], s=20)
        info_text = ax.text(0.02, 0.98, '', transform=ax.transAxes, va='top', bbox=dict(
            boxstyle='round', facecolor='white', alpha=0.8))

        # 3. Читаем и рисуем кадры
        for idx, (iter_num, fpath, _) in enumerate(files):
            t, pos, types = read_single_frame(fpath, main_params['N'])
            if t is None:
                continue

            scatter.set_offsets(pos)
            colors = np.where(types == b'a', 'red', 'blue')
            scatter.set_color(colors)

            info_text.set_text(
                f"Time: {t:.2f} s\nN={main_params['N']}\nIter: {iter_num}")

            # Рендерим текущий кадр в память (чтобы не было белых экранов)
            fig.canvas.draw()
            buf = io.BytesIO()
            fig.savefig(buf, format='png', dpi=100)
            buf.seek(0)
            frames_for_gif.append(Image.open(buf))

            if (idx + 1) % 20 == 0:
                print(f"  Отрисовано {idx + 1}/{len(files)}_{fpath}")

        plt.close(fig)

        # 4. Склеиваем идеальную гифку через PIL
        # duration=50 означает 50 мс на кадр.
        # То есть смена кадров будет идти гладко в реальном физическом времени.
        out_gif = os.path.join(target_folder, f"{base_name}.gif")
        frames_for_gif[0].save(
            out_gif,
            save_all=True,
            append_images=frames_for_gif[1:],
            duration=50,
            loop=0
        )
        print(f"Готово! Сохранено в: {out_gif}\n")
