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
        'base_name': f"sim_N{match.group(1)}_L{match.group(2)}_alpha{match.group(4)}_sigma{match.group(5)}_seed{match.group(6)}"
    }


def read_single_frame(fpath, N):
    agent_dtype = np.dtype([('x', '<f4'), ('y', '<f4'), ('r', '<f4'),
                           ('vx', '<f4'), ('vy', '<f4'), ('type', 'S1')])
    with open(fpath, 'rb') as f:
        t_bytes = f.read(8)
        if len(t_bytes) < 8:
            return None, None, None
        ag_bytes = f.read(N * agent_dtype.itemsize)
        agents_data = np.frombuffer(ag_bytes, dtype=agent_dtype)
        velocities = np.column_stack((agents_data['vx'], agents_data['vy']))
        return struct.unpack('<d', t_bytes)[0], velocities, agents_data['type']


if __name__ == "__main__":
    target_folder = "../build/"
    bin_files = glob.glob(os.path.join(target_folder, "*.bin"))

    simulations = defaultdict(list)
    for f in bin_files:
        p = parse_filename(f)
        if p:
            simulations[p['base_name']].append((p['it'], f, p))

    for base_name, files in simulations.items():
        files.sort(key=lambda x: x[0])
        main_params = files[0][2]
        N = main_params['N']

        print(f"Сборка GIF распределений для: {base_name}")

        all_speeds = []
        max_speed = 0.1
        max_density = 0.1

        # ШАГ 1: Читаем все файлы заранее, чтобы зафиксировать оси X и Y
        # Если оси будут прыгать, гифку будет невозможно анализировать визуально
        for iter_num, fpath, _ in files:
            t, vels, types = read_single_frame(fpath, N)
            if t is None:
                continue

            # ФИЛЬТРУЕМ: Оставляем только АКТивных ('a') агентов
            active_mask = (types == b'a')
            active_vels = vels[active_mask]

            if len(active_vels) == 0:
                all_speeds.append((t, iter_num, []))
                continue

            speeds = np.linalg.norm(active_vels, axis=1)
            all_speeds.append((t, iter_num, speeds))

            # Ищем максимальную скорость для оси X
            curr_max_v = np.max(speeds)
            if curr_max_v > max_speed:
                max_speed = curr_max_v

            # Ищем максимальную "высоту" гистограммы (плотность) для оси Y
            # Мы пропускаем 0-й кадр, так как там все скорости обычно одинаковые,
            # что дает 1 бесконечно высокий столб, который сломает масштаб всей гифки
            if iter_num > 5:
                hist, _ = np.histogram(
                    speeds, bins=30, range=(0, max_speed), density=True)
                curr_max_d = np.max(hist)
                if curr_max_d > max_density:
                    max_density = curr_max_d

        # ШАГ 2: Рисуем гифку
        frames_for_gif = []
        fig, ax = plt.subplots(figsize=(8, 6))

        for idx, (t, iter_num, speeds) in enumerate(all_speeds):
            ax.clear()

            if len(speeds) > 0:
                ax.hist(speeds, bins=30, range=(0, max_speed), density=True,
                        color='purple', alpha=0.7, edgecolor='black')
                # Расчет эффективной температуры (средний квадрат скорости)
                T_eff = np.mean(speeds**2)
                
                # Построение теоретической кривой Максвелла-Больцмана
                if T_eff > 1e-6: # Защита от деления на ноль на первых кадрах
                    v_vals = np.linspace(0, max_speed * 1.1, 200)
                    pdf_vals = (2 * v_vals / T_eff) * np.exp(-(v_vals**2) / T_eff)
                    ax.plot(v_vals, pdf_vals, color='red', linewidth=2.5, 
                            label=f'Maxwell-Boltzmann ($T_{{eff}}$={T_eff:.2f})')
                    ax.legend(loc='upper right', fontsize=11)

            # Фиксируем оси, чтобы график не дергался!
            ax.set_xlim(0, max_speed * 1.1)
            ax.set_ylim(0, max_density * 1.1)

            ax.set_title(
                f"Распределение скоростей (только АКТИВНЫЕ)\nTime: {t:.2f} s | Iter: {iter_num}", fontsize=14)
            ax.set_xlabel("Модуль скорости $|v|$", fontsize=12)
            ax.set_ylabel("Плотность вероятности $P(|v|)$", fontsize=12)
            ax.grid(True, alpha=0.4)

            # Сохраняем в буфер памяти (без белых экранов)
            fig.canvas.draw()
            buf = io.BytesIO()
            fig.savefig(buf, format='png', dpi=100, bbox_inches='tight')
            buf.seek(0)

            img = Image.open(buf)
            img.load()
            frames_for_gif.append(img)
            buf.close()

            if (idx + 1) % 20 == 0:
                print(
                    f"  Отрендерено {idx + 1}/{len(all_speeds)} кадров для плотности")

        plt.close(fig)

        # ШАГ 3: Сохраняем как GIF
        if frames_for_gif:
            out_gif = os.path.join(
                target_folder, f"distribution_{base_name}.gif")
            frames_for_gif[0].save(
                out_gif,
                save_all=True,
                append_images=frames_for_gif[1:],
                duration=50,  # 50 мс на кадр (как в основной анимации)
                loop=0
            )
            print(f"Готово! Анимация плотностей сохранена: {out_gif}\n")
