from pathlib import Path
import math
import random
import wave
import struct

PROJECT_DIR = Path(__file__).resolve().parent
ASSET_DIR = PROJECT_DIR / 'assets' / 'sfx'
ASSET_DIR.mkdir(parents=True, exist_ok=True)
SAMPLE_RATE = 22050


def clamp(v, lo=-1.0, hi=1.0):
    return lo if v < lo else hi if v > hi else v


def env(t, duration, attack=0.01, decay=0.08, sustain=0.55, release=0.12):
    if t < 0 or t > duration:
        return 0.0
    if t < attack:
        return t / max(attack, 1e-6)
    t -= attack
    if t < decay:
        return 1.0 + (sustain - 1.0) * (t / max(decay, 1e-6))
    sustain_time = max(0.0, duration - attack - decay - release)
    if t < sustain_time:
        return sustain
    t -= sustain_time
    return sustain * max(0.0, 1.0 - t / max(release, 1e-6))


def tone(freq, t, phase=0.0):
    return math.sin(2.0 * math.pi * freq * t + phase)


def noise():
    return random.uniform(-1.0, 1.0)


def render(name, duration, fn):
    frames = []
    count = int(duration * SAMPLE_RATE)
    for i in range(count):
        t = i / SAMPLE_RATE
        s = clamp(fn(t, duration))
        frames.append(struct.pack('<h', int(s * 32767)))
    with wave.open(str(ASSET_DIR / name), 'wb') as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(SAMPLE_RATE)
        out.writeframes(b''.join(frames))


random.seed(7)

render('ui_move.wav', 0.08, lambda t, d: 0.22 * env(t, d, 0.002, 0.02, 0.0, 0.05) * (tone(950, t) + 0.6 * tone(1450, t)))
render('ui_accept.wav', 0.16, lambda t, d: 0.24 * env(t, d, 0.002, 0.04, 0.2, 0.06) * (tone(640, t) + 0.8 * tone(960, t)))
render('ui_deny.wav', 0.18, lambda t, d: 0.22 * env(t, d, 0.002, 0.05, 0.2, 0.08) * (tone(320, t) + 0.5 * tone(250, t)))

render('sword_light.wav', 0.16, lambda t, d: 0.42 * env(t, d, 0.001, 0.04, 0.12, 0.08) * (0.5 * noise() + 0.5 * tone(700 - 180 * t, t)))
render('sword_heavy.wav', 0.28, lambda t, d: 0.46 * env(t, d, 0.002, 0.08, 0.16, 0.10) * (0.45 * noise() + 0.55 * tone(240 + 120 * t, t)))
render('hit.wav', 0.12, lambda t, d: 0.46 * env(t, d, 0.001, 0.03, 0.0, 0.06) * (0.75 * noise() + 0.25 * tone(180, t)))
render('crit.wav', 0.22, lambda t, d: 0.34 * env(t, d, 0.001, 0.04, 0.2, 0.10) * (tone(880, t) + 0.6 * tone(1320, t) + 0.25 * noise()))
render('dash.wav', 0.15, lambda t, d: 0.34 * env(t, d, 0.001, 0.03, 0.0, 0.09) * (0.7 * noise() + 0.3 * tone(420 + 240 * t, t)))
render('burst.wav', 0.34, lambda t, d: 0.36 * env(t, d, 0.002, 0.08, 0.18, 0.18) * (0.55 * noise() + 0.45 * tone(180 + 60 * math.sin(t * 18), t)))
render('banner.wav', 0.24, lambda t, d: 0.28 * env(t, d, 0.003, 0.04, 0.26, 0.10) * (tone(220, t) + 0.7 * tone(330, t)))
render('quest.wav', 0.42, lambda t, d: 0.20 * env(t, d, 0.004, 0.06, 0.3, 0.14) * (tone(440, t) + 0.8 * tone(660, t) + 0.5 * tone(880, t)))
render('pet.wav', 0.18, lambda t, d: 0.22 * env(t, d, 0.002, 0.03, 0.14, 0.08) * (tone(720, t) + 0.3 * tone(1080, t)))
render('heal.wav', 0.32, lambda t, d: 0.18 * env(t, d, 0.006, 0.08, 0.3, 0.14) * (tone(560 + 120 * t, t) + 0.6 * tone(840 + 160 * t, t)))
render('boss_intro.wav', 0.60, lambda t, d: 0.28 * env(t, d, 0.004, 0.12, 0.32, 0.24) * (tone(110, t) + 0.8 * tone(165, t) + 0.35 * noise()))
render('travel.wav', 0.38, lambda t, d: 0.22 * env(t, d, 0.004, 0.08, 0.24, 0.16) * (tone(420 + 260 * t, t) + 0.5 * tone(690 + 180 * t, t)))
render('reward.wav', 0.36, lambda t, d: 0.24 * env(t, d, 0.003, 0.06, 0.18, 0.16) * (tone(523, t) + 0.8 * tone(784, t) + 0.45 * tone(1046, t)))
render('game_over.wav', 0.72, lambda t, d: 0.24 * env(t, d, 0.004, 0.10, 0.28, 0.30) * (tone(220 - 40 * t, t) + 0.7 * tone(164 - 22 * t, t)))

print('Sound assets written to', ASSET_DIR)
