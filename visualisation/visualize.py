import os
import re
import struct
import glob
import io
import numpy as np
import matplotlib.pyplot as plt
from PIL import Image
from collections import defaultdict
from matplotlib.patches import Circle


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
            return None, None, None, None, None

        current_time = struct.unpack('<d', t_bytes)[0]
        ag_bytes = f.read(N * agent_dtype.itemsize)

        agents_data = np.frombuffer(ag_bytes, dtype=agent_dtype)
        positions = np.column_stack((agents_data['x'], agents_data['y']))
        velocities = np.column_stack((agents_data['vx'], agents_data['vy']))

        return current_time, positions, agents_data['r'], velocities, agents_data['type']


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

        print(f"Сборка GIF: {base_name} ({len(files)} кадров)")
        frames_for_gif = []

        fig, ax = plt.subplots(figsize=(8, 8))

        for idx, (iter_num, fpath, _) in enumerate(files):
            t, pos, rads, vels, types = read_single_frame(fpath, N)
            if t is None:
                continue

            ax.clear()
            ax.set_xlim(0, L)
            ax.set_ylim(0, L)
            ax.set_aspect('equal')
            ax.set_xticks([])
            ax.set_yticks([])

            for i in range(N):
                color = '#ff3333' if types[i] == b'a' else '#3333ff'
                circle = Circle((pos[i, 0], pos[i, 1]), rads[i],
                                facecolor=color, edgecolor='black', linewidth=0.5)
                ax.add_patch(circle)

            v_mag = np.linalg.norm(vels, axis=1)
            moving_mask = v_mag > 1e-4

            dx = np.zeros_like(vels[:, 0])
            dy = np.zeros_like(vels[:, 1])

            arrow_length = rads * 1.2

            dx[moving_mask] = (vels[moving_mask, 0] /
                               v_mag[moving_mask]) * arrow_length[moving_mask]
            dy[moving_mask] = (vels[moving_mask, 1] /
                               v_mag[moving_mask]) * arrow_length[moving_mask]

            ax.quiver(pos[:, 0], pos[:, 1], dx, dy,
                      angles='xy', scale_units='xy', scale=1,
                      color='white', pivot='mid', width=0.004, headwidth=4)

            ax.text(0.02, 0.98, f"Time: {t:.2f} s\nN={N}\nR={rads[0]:.1f}\nL={L}",
                    transform=ax.transAxes, va='top', bbox=dict(boxstyle='round', facecolor='white', alpha=0.8))

            fig.canvas.draw()
            buf = io.BytesIO()
            fig.savefig(buf, format='png', dpi=100,
                        bbox_inches='tight', pad_inches=0.1)
            buf.seek(0)

            img = Image.open(buf)
            img.load()
            frames_for_gif.append(img)
            buf.close()

        plt.close(fig)

        if frames_for_gif:
            out_gif = os.path.join(target_folder, f"{base_name}.gif")
            frames_for_gif[0].save(
                out_gif, save_all=True, append_images=frames_for_gif[1:], duration=50, loop=0)
            print(f"Готово! {out_gif}\n")
