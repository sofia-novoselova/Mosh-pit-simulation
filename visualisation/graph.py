import os
import glob
import numpy as np
import matplotlib.pyplot as plt
# Импортируем ваши же функции чтения
from visualize import parse_filename, read_single_frame
from collections import defaultdict

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
        L = main_params['L']
        N = main_params['N']

        print(f"Считаю физику для: {base_name}...")

        times = []
        temperatures = []
        angular_momentums = []
        velocities = []

        center = np.array([L / 2.0, L / 2.0])

        for iter_num, fpath, _ in files:
            t, pos, rads, vels, types = read_single_frame(fpath, N)
            if t is None:
                continue

            times.append(t)

            # 1. Температура (кинетическая энергия ~ <v^2>)
            v_squared = np.sum(vels**2, axis=1)
            # Коэффициент 1/2 можно убрать, но для вида кинетической энергии оставим
            T = np.mean(v_squared) / 2.0
            temperatures.append(T)

            # 2. Момент импульса (L_z = r x v) относительно центра арены
            r_rel = pos - center
            # cross_product для 2D: x*v_y - y*v_x
            lz = r_rel[:, 0] * vels[:, 1] - r_rel[:, 1] * vels[:, 0]
            L_z_total = np.mean(lz)  # Средний момент вращения
            angular_momentums.append(L_z_total)

            # Сохраняем скорости последнего кадра для распределения Больцмана
            if iter_num == files[1][0]:
                velocities = np.linalg.norm(vels, axis=1)

        # === РИСОВАНИЕ ГРАФИКОВ ===
        fig, axs = plt.subplots(1, 3, figsize=(18, 5))
        fig.suptitle(
            f"Мошпит Макро-Параметры: N={N}, $\\alpha$={main_params['alpha']}, $\\sigma$={main_params['sigma']}", fontsize=16)

        # График 1: Температура
        axs[0].plot(times, temperatures, color='red', lw=2)
        axs[0].set_title(
            'Температура $T \propto \\langle v^2 \\rangle$', fontsize=14)
        axs[0].set_xlabel('Время (s)')
        axs[0].set_ylabel('Температура')
        axs[0].grid(True, alpha=0.5)

        # График 2: Момент импульса
        axs[1].plot(times, angular_momentums, color='blue', lw=2)
        axs[1].set_title('Удельный момент импульса $L_z$', fontsize=14)
        axs[1].set_xlabel('Время (s)')
        axs[1].set_ylabel('$L_z$')
        axs[1].axhline(0, color='black', lw=1, ls='--')
        axs[1].grid(True, alpha=0.5)

        # График 3: Распределение скоростей (Больцман)
        axs[2].hist(velocities, bins=30, density=True,
                    color='purple', alpha=0.7, edgecolor='black')
        axs[2].set_title(
            'Распределение скоростей в начале', fontsize=14)
        axs[2].set_xlabel('Скорость $|v|$')
        axs[2].set_ylabel('Плотность вероятности')
        axs[2].grid(True, alpha=0.5)

        plt.tight_layout()
        out_plot = os.path.join(target_folder, f"physics_{base_name}.png")
        plt.savefig(out_plot, dpi=150)
        plt.close()
        print(f"Графики сохранены: {out_plot}\n")
