from PIL import Image, ImageDraw
from pathlib import Path

ASSET_DIR = Path('/home/user/assets')
ASSET_DIR.mkdir(parents=True, exist_ok=True)

WORLDS = {
    'crownheart': {
        'tiles': {
            'grass': (153, 196, 79, 255),
            'grass_dark': (87, 135, 52, 255),
            'grass_alt': (145, 188, 73, 255),
            'grass_alt_light': (175, 210, 92, 255),
            'path': (240, 219, 156, 255),
            'path_detail': (214, 192, 128, 255),
            'path_outline': (188, 165, 105, 255),
            'flowers': (255, 244, 245, 255),
            'flowers_core': (255, 220, 80, 255),
        },
        'props': {
            'leaf': (65, 181, 84, 255),
            'leaf_dark': (39, 113, 48, 255),
            'bush': (76, 171, 74, 255),
            'bush_dark': (45, 116, 49, 255),
            'trunk': (121, 84, 52, 255),
            'trunk_dark': (74, 55, 34, 255),
            'rock': (198, 191, 172, 255),
            'rock_light': (225, 220, 203, 255),
            'rock_dark': (123, 116, 99, 255),
            'hut': (160, 128, 74, 255),
            'hut_roof': (182, 152, 75, 255),
            'hut_dark': (96, 73, 42, 255),
            'tower': (184, 184, 190, 255),
            'tower_dark': (96, 96, 102, 255),
            'banner': (183, 72, 61, 255),
            'banner_dark': (102, 44, 38, 255),
            'castle': (178, 178, 185, 255),
            'castle_dark': (92, 92, 97, 255),
            'castle_banner': (92, 133, 207, 255),
            'castle_banner_dark': (55, 76, 121, 255),
            'chest': (184, 123, 68, 255),
            'chest_base': (151, 99, 56, 255),
            'gold': (223, 188, 82, 255),
            'gold_dark': (130, 108, 37, 255),
            'fence': (143, 104, 67, 255),
            'fence_dark': (88, 62, 40, 255),
            'sign': (227, 213, 168, 255),
            'sign_dark': (112, 98, 70, 255),
        },
        'enemy': {
            'main': (138, 110, 72, 255),
            'dark': (78, 56, 36, 255),
            'accent': (194, 62, 58, 255),
            'glow': (244, 214, 118, 255),
        },
    },
    'frostveil': {
        'tiles': {
            'grass': (222, 236, 248, 255),
            'grass_dark': (157, 184, 214, 255),
            'grass_alt': (208, 226, 244, 255),
            'grass_alt_light': (240, 247, 255, 255),
            'path': (232, 240, 252, 255),
            'path_detail': (202, 216, 236, 255),
            'path_outline': (172, 190, 214, 255),
            'flowers': (250, 252, 255, 255),
            'flowers_core': (194, 226, 255, 255),
        },
        'props': {
            'leaf': (178, 212, 225, 255),
            'leaf_dark': (112, 148, 171, 255),
            'bush': (164, 198, 214, 255),
            'bush_dark': (108, 138, 160, 255),
            'trunk': (113, 94, 82, 255),
            'trunk_dark': (76, 60, 50, 255),
            'rock': (199, 210, 224, 255),
            'rock_light': (234, 242, 250, 255),
            'rock_dark': (128, 142, 158, 255),
            'hut': (155, 164, 180, 255),
            'hut_roof': (206, 220, 236, 255),
            'hut_dark': (96, 108, 126, 255),
            'tower': (214, 224, 236, 255),
            'tower_dark': (126, 140, 158, 255),
            'banner': (116, 160, 220, 255),
            'banner_dark': (64, 94, 136, 255),
            'castle': (206, 214, 228, 255),
            'castle_dark': (122, 132, 149, 255),
            'castle_banner': (168, 204, 255, 255),
            'castle_banner_dark': (92, 128, 176, 255),
            'chest': (170, 188, 212, 255),
            'chest_base': (136, 152, 176, 255),
            'gold': (236, 246, 255, 255),
            'gold_dark': (146, 178, 214, 255),
            'fence': (154, 166, 186, 255),
            'fence_dark': (92, 104, 124, 255),
            'sign': (234, 242, 252, 255),
            'sign_dark': (136, 154, 176, 255),
        },
        'enemy': {
            'main': (152, 188, 228, 255),
            'dark': (92, 116, 150, 255),
            'accent': (210, 236, 255, 255),
            'glow': (182, 226, 255, 255),
        },
    },
    'sunscar': {
        'tiles': {
            'grass': (224, 198, 118, 255),
            'grass_dark': (181, 139, 72, 255),
            'grass_alt': (214, 184, 103, 255),
            'grass_alt_light': (238, 208, 126, 255),
            'path': (243, 218, 157, 255),
            'path_detail': (215, 183, 113, 255),
            'path_outline': (181, 146, 88, 255),
            'flowers': (255, 237, 184, 255),
            'flowers_core': (255, 196, 89, 255),
        },
        'props': {
            'leaf': (130, 162, 88, 255),
            'leaf_dark': (85, 108, 54, 255),
            'bush': (168, 138, 72, 255),
            'bush_dark': (112, 84, 40, 255),
            'trunk': (130, 90, 46, 255),
            'trunk_dark': (83, 55, 28, 255),
            'rock': (204, 178, 132, 255),
            'rock_light': (238, 213, 165, 255),
            'rock_dark': (138, 108, 68, 255),
            'hut': (193, 148, 84, 255),
            'hut_roof': (226, 178, 96, 255),
            'hut_dark': (118, 82, 38, 255),
            'tower': (214, 178, 128, 255),
            'tower_dark': (138, 102, 62, 255),
            'banner': (216, 128, 62, 255),
            'banner_dark': (128, 72, 34, 255),
            'castle': (209, 182, 144, 255),
            'castle_dark': (128, 95, 58, 255),
            'castle_banner': (235, 188, 73, 255),
            'castle_banner_dark': (144, 110, 38, 255),
            'chest': (196, 124, 58, 255),
            'chest_base': (164, 98, 44, 255),
            'gold': (247, 210, 82, 255),
            'gold_dark': (150, 110, 30, 255),
            'fence': (167, 116, 68, 255),
            'fence_dark': (101, 66, 36, 255),
            'sign': (244, 223, 176, 255),
            'sign_dark': (143, 110, 62, 255),
        },
        'enemy': {
            'main': (190, 140, 78, 255),
            'dark': (116, 74, 34, 255),
            'accent': (250, 204, 88, 255),
            'glow': (255, 236, 160, 255),
        },
    },
    'mirethorn': {
        'tiles': {
            'grass': (132, 156, 102, 255),
            'grass_dark': (76, 97, 58, 255),
            'grass_alt': (118, 143, 90, 255),
            'grass_alt_light': (154, 178, 120, 255),
            'path': (166, 148, 104, 255),
            'path_detail': (128, 113, 78, 255),
            'path_outline': (95, 80, 56, 255),
            'flowers': (176, 198, 136, 255),
            'flowers_core': (218, 232, 149, 255),
        },
        'props': {
            'leaf': (84, 124, 70, 255),
            'leaf_dark': (46, 78, 38, 255),
            'bush': (98, 132, 82, 255),
            'bush_dark': (57, 83, 47, 255),
            'trunk': (95, 72, 49, 255),
            'trunk_dark': (59, 43, 29, 255),
            'rock': (132, 140, 114, 255),
            'rock_light': (169, 180, 150, 255),
            'rock_dark': (82, 90, 73, 255),
            'hut': (110, 120, 84, 255),
            'hut_roof': (84, 96, 60, 255),
            'hut_dark': (62, 72, 48, 255),
            'tower': (134, 130, 110, 255),
            'tower_dark': (86, 82, 68, 255),
            'banner': (140, 188, 102, 255),
            'banner_dark': (76, 108, 54, 255),
            'castle': (124, 126, 106, 255),
            'castle_dark': (78, 79, 66, 255),
            'castle_banner': (162, 196, 112, 255),
            'castle_banner_dark': (92, 118, 62, 255),
            'chest': (126, 110, 66, 255),
            'chest_base': (100, 84, 46, 255),
            'gold': (208, 198, 122, 255),
            'gold_dark': (116, 104, 52, 255),
            'fence': (118, 92, 58, 255),
            'fence_dark': (72, 54, 34, 255),
            'sign': (192, 186, 144, 255),
            'sign_dark': (101, 94, 67, 255),
        },
        'enemy': {
            'main': (104, 132, 84, 255),
            'dark': (60, 82, 48, 255),
            'accent': (168, 208, 122, 255),
            'glow': (216, 232, 148, 255),
        },
    },
}

WEAPON_COLORS = [
    (200, 200, 200, 255), (130, 130, 130, 255), (59, 146, 209, 255), (255, 161, 0, 255),
    (102, 191, 255, 255), (0, 121, 241, 255), (230, 41, 55, 255), (0, 158, 47, 255),
    (59, 146, 209, 255), (200, 122, 255, 255), (253, 249, 0, 255), (135, 60, 190, 255),
    (255, 203, 0, 255), (0, 228, 48, 255), (121, 94, 161, 255), (255, 255, 255, 255),
    (191, 86, 101, 255), (88, 128, 204, 255), (100, 110, 128, 255), (132, 182, 236, 255),
    (202, 236, 255, 255), (150, 208, 255, 255), (194, 143, 80, 255), (226, 182, 86, 255),
    (235, 188, 73, 255), (98, 122, 72, 255), (118, 164, 108, 255), (164, 208, 132, 255),
    (160, 178, 118, 255),
]


def rounded(draw, box, radius, fill, outline=None, width=2):
    draw.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)


def make_tile_atlas(world_name, palette):
    cell = 64
    img = Image.new('RGBA', (cell * 4, cell), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    t = palette['tiles']

    x = 0
    d.rectangle((x, 0, x + cell, cell), fill=t['grass'])
    for px, py in [(8, 10), (19, 20), (39, 14), (49, 32), (22, 49), (47, 50)]:
        d.line((x + px, py, x + px + 4, py - 5), fill=t['grass_dark'], width=2)
        d.line((x + px, py, x + px - 3, py - 5), fill=t['grass_dark'], width=2)

    x = cell
    d.rectangle((x, 0, x + cell, cell), fill=t['path'])
    for px, py in [(10, 12), (25, 18), (45, 15), (15, 40), (38, 45), (52, 31)]:
        d.ellipse((x + px, py, x + px + 6, py + 4), fill=t['path_detail'], outline=t['path_outline'])

    x = cell * 2
    d.rectangle((x, 0, x + cell, cell), fill=t['grass_alt'])
    for px, py in [(10, 8), (30, 20), (20, 42), (48, 16), (44, 48)]:
        d.ellipse((x + px, py, x + px + 8, py + 8), fill=t['grass_alt_light'])
    for px, py in [(15, 55), (39, 37), (55, 21)]:
        d.line((x + px, py, x + px + 4, py - 5), fill=t['grass_dark'], width=2)
        d.line((x + px, py, x + px - 3, py - 5), fill=t['grass_dark'], width=2)

    x = cell * 3
    d.rectangle((x, 0, x + cell, cell), fill=t['grass'])
    for px, py in [(16, 16), (42, 25), (27, 44)]:
        d.ellipse((x + px, py, x + px + 4, py + 4), fill=t['flowers'])
        d.ellipse((x + px + 5, py + 3, x + px + 9, py + 7), fill=t['flowers'])
        d.ellipse((x + px + 2, py + 6, x + px + 6, py + 10), fill=t['flowers_core'])

    img.save(ASSET_DIR / f'world_tiles_{world_name}.png')
    if world_name == 'crownheart':
        img.save(ASSET_DIR / 'world_tiles.png')


def make_prop_atlas(world_name, palette):
    cell = 96
    cols, rows = 5, 2
    img = Image.new('RGBA', (cell * cols, cell * rows), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    p = palette['props']

    def box(i):
        return (i % cols) * cell, (i // cols) * cell

    x, y = box(0)
    rounded(d, (x + 42, y + 42, x + 54, y + 80), 4, fill=p['trunk'], outline=p['trunk_dark'], width=2)
    for ox, oy, r in [(48, 28, 22), (34, 35, 18), (62, 35, 18), (48, 45, 20)]:
        d.ellipse((x + ox - r, y + oy - r, x + ox + r, y + oy + r), fill=p['leaf'], outline=p['leaf_dark'], width=3)

    x, y = box(1)
    for ox, oy, r in [(34, 58, 16), (50, 52, 20), (66, 58, 16)]:
        d.ellipse((x + ox - r, y + oy - r, x + ox + r, y + oy + r), fill=p['bush'], outline=p['bush_dark'], width=3)

    x, y = box(2)
    d.polygon([(x + 18, y + 70), (x + 36, y + 22), (x + 76, y + 30), (x + 82, y + 68), (x + 52, y + 80)], fill=p['rock'], outline=p['rock_dark'])
    d.polygon([(x + 30, y + 62), (x + 44, y + 36), (x + 69, y + 42), (x + 59, y + 70)], fill=p['rock_light'], outline=p['rock_dark'])

    x, y = box(3)
    rounded(d, (x + 24, y + 42, x + 72, y + 78), 8, fill=p['hut'], outline=p['hut_dark'], width=3)
    d.polygon([(x + 16, y + 46), (x + 48, y + 16), (x + 80, y + 46)], fill=p['hut_roof'], outline=p['hut_dark'])
    rounded(d, (x + 42, y + 52, x + 54, y + 78), 4, fill=p['trunk'], outline=p['trunk_dark'], width=2)

    x, y = box(4)
    rounded(d, (x + 34, y + 32, x + 62, y + 82), 10, fill=p['tower'], outline=p['tower_dark'], width=3)
    d.polygon([(x + 30, y + 38), (x + 48, y + 14), (x + 66, y + 38)], fill=p['trunk'], outline=p['trunk_dark'])
    d.line((x + 60, y + 16, x + 60, y + 2), fill=p['tower_dark'], width=2)
    d.polygon([(x + 60, y + 2), (x + 76, y + 8), (x + 60, y + 14)], fill=p['banner'], outline=p['banner_dark'])

    x, y = box(5)
    rounded(d, (x + 22, y + 38, x + 74, y + 82), 8, fill=p['castle'], outline=p['castle_dark'], width=3)
    rounded(d, (x + 6, y + 32, x + 26, y + 84), 8, fill=p['castle'], outline=p['castle_dark'], width=3)
    rounded(d, (x + 70, y + 32, x + 90, y + 84), 8, fill=p['castle'], outline=p['castle_dark'], width=3)
    rounded(d, (x + 38, y + 22, x + 58, y + 78), 8, fill=p['castle'], outline=p['castle_dark'], width=3)
    for cx in [16, 48, 80]:
        d.polygon([(x + cx - 10, y + 32 if cx != 48 else y + 22), (x + cx, y + (5 if cx != 48 else 0)), (x + cx + 10, y + 32 if cx != 48 else y + 22)], fill=p['castle_banner'], outline=p['castle_banner_dark'])
    rounded(d, (x + 42, y + 56, x + 54, y + 82), 4, fill=p['trunk'], outline=p['trunk_dark'], width=2)

    x, y = box(6)
    rounded(d, (x + 24, y + 42, x + 72, y + 68), 8, fill=p['chest_base'], outline=p['trunk_dark'], width=3)
    rounded(d, (x + 20, y + 26, x + 76, y + 48), 10, fill=p['chest'], outline=p['trunk_dark'], width=3)
    rounded(d, (x + 44, y + 34, x + 52, y + 60), 4, fill=p['gold'], outline=p['gold_dark'], width=2)

    x, y = box(7)
    for i in range(5):
        rounded(d, (x + 8 + i * 16, y + 46, x + 18 + i * 16, y + 74), 4, fill=p['fence'], outline=p['fence_dark'], width=2)
    d.line((x + 8, y + 70, x + 86, y + 48), fill=p['fence_dark'], width=4)
    d.line((x + 8, y + 48, x + 86, y + 70), fill=p['fence_dark'], width=4)

    x, y = box(8)
    for i in range(5):
        rounded(d, (x + 40, y + 8 + i * 16, x + 58, y + 18 + i * 16), 4, fill=p['fence'], outline=p['fence_dark'], width=2)
    d.line((x + 44, y + 8, x + 56, y + 88), fill=p['fence_dark'], width=4)
    d.line((x + 56, y + 8, x + 44, y + 88), fill=p['fence_dark'], width=4)

    x, y = box(9)
    d.rectangle((x + 44, y + 28, x + 52, y + 82), fill=p['trunk'], outline=p['trunk_dark'], width=2)
    rounded(d, (x + 22, y + 22, x + 72, y + 42), 5, fill=p['sign'], outline=p['sign_dark'], width=3)

    img.save(ASSET_DIR / f'props_atlas_{world_name}.png')
    if world_name == 'crownheart':
        img.save(ASSET_DIR / 'props_atlas.png')


def make_actor_atlas():
    cell = 64
    cols, rows = 4, 2
    img = Image.new('RGBA', (cell * cols, cell * rows), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    def box(i):
        return (i % cols) * cell, (i // cols) * cell

    x, y = box(0)
    d.polygon([(x + 32, y + 10), (x + 18, y + 44), (x + 46, y + 44)], fill=(180, 68, 61, 255), outline=(91, 43, 40, 255))
    rounded(d, (x + 22, y + 24, x + 42, y + 50), 8, fill=(100, 130, 179, 255), outline=(55, 74, 104, 255), width=2)
    d.ellipse((x + 24, y + 8, x + 40, y + 24), fill=(230, 219, 200, 255), outline=(102, 88, 72, 255))
    d.line((x + 32, y + 28, x + 54, y + 16), fill=(120, 87, 54, 255), width=3)

    x, y = box(1)
    d.ellipse((x + 12, y + 16, x + 52, y + 48), fill=(89, 193, 96, 255), outline=(45, 119, 55, 255), width=3)
    d.ellipse((x + 20, y + 26, x + 28, y + 34), fill=(255, 255, 255, 255), outline=(0, 0, 0, 255))
    d.ellipse((x + 36, y + 26, x + 44, y + 34), fill=(255, 255, 255, 255), outline=(0, 0, 0, 255))

    x, y = box(2)
    rounded(d, (x + 16, y + 18, x + 48, y + 50), 12, fill=(175, 124, 74, 255), outline=(88, 61, 35, 255), width=3)
    d.polygon([(x + 16, y + 26), (x + 8, y + 18), (x + 18, y + 18)], fill=(175, 124, 74, 255), outline=(88, 61, 35, 255))
    d.polygon([(x + 48, y + 26), (x + 56, y + 18), (x + 46, y + 18)], fill=(175, 124, 74, 255), outline=(88, 61, 35, 255))
    d.ellipse((x + 22, y + 28, x + 28, y + 34), fill=(255, 255, 255, 255))
    d.ellipse((x + 36, y + 28, x + 42, y + 34), fill=(255, 255, 255, 255))

    x, y = box(3)
    d.polygon([(x + 8, y + 34), (x + 22, y + 22), (x + 24, y + 36), (x + 16, y + 44)], fill=(117, 92, 160, 255), outline=(64, 49, 88, 255))
    d.polygon([(x + 56, y + 34), (x + 42, y + 22), (x + 40, y + 36), (x + 48, y + 44)], fill=(117, 92, 160, 255), outline=(64, 49, 88, 255))
    d.ellipse((x + 22, y + 18, x + 42, y + 42), fill=(137, 110, 178, 255), outline=(64, 49, 88, 255), width=3)

    x, y = box(4)
    rounded(d, (x + 16, y + 14, x + 48, y + 48), 8, fill=(156, 156, 163, 255), outline=(90, 90, 96, 255), width=3)
    rounded(d, (x + 10, y + 24, x + 20, y + 48), 5, fill=(156, 156, 163, 255), outline=(90, 90, 96, 255), width=3)
    rounded(d, (x + 44, y + 24, x + 54, y + 48), 5, fill=(156, 156, 163, 255), outline=(90, 90, 96, 255), width=3)

    x, y = box(5)
    rounded(d, (x + 12, y + 10, x + 52, y + 52), 12, fill=(171, 70, 70, 255), outline=(88, 34, 34, 255), width=3)
    d.polygon([(x + 18, y + 16), (x + 12, y + 2), (x + 22, y + 12)], fill=(171, 70, 70, 255), outline=(88, 34, 34, 255))
    d.polygon([(x + 46, y + 16), (x + 52, y + 2), (x + 42, y + 12)], fill=(171, 70, 70, 255), outline=(88, 34, 34, 255))
    d.ellipse((x + 20, y + 24, x + 28, y + 32), fill=(255, 255, 255, 255))
    d.ellipse((x + 36, y + 24, x + 44, y + 32), fill=(255, 255, 255, 255))

    x, y = box(6)
    rounded(d, (x + 18, y + 28, x + 46, y + 48), 8, fill=(118, 126, 152, 255), outline=(62, 70, 94, 255), width=3)
    d.rectangle((x + 28, y + 14, x + 36, y + 28), fill=(118, 126, 152, 255), outline=(62, 70, 94, 255))
    d.rectangle((x + 36, y + 18, x + 52, y + 24), fill=(93, 102, 130, 255), outline=(62, 70, 94, 255))

    x, y = box(7)
    d.rectangle((x + 30, y + 10, x + 34, y + 48), fill=(118, 89, 62, 255))
    d.polygon([(x + 34, y + 12), (x + 54, y + 18), (x + 34, y + 28)], fill=(183, 72, 61, 255), outline=(100, 45, 40, 255))

    img.save(ASSET_DIR / 'actors_atlas.png')


def make_enemy_atlas(world_name, palette):
    cell = 64
    cols, rows = 4, 2
    img = Image.new('RGBA', (cell * cols, cell * rows), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    e = palette['enemy']

    def box(i):
        return (i % cols) * cell, (i // cols) * cell

    x, y = box(0)
    d.polygon([(x + 10, y + 44), (x + 18, y + 28), (x + 34, y + 24), (x + 52, y + 34), (x + 48, y + 48), (x + 28, y + 50)], fill=e['main'], outline=e['dark'])
    d.polygon([(x + 18, y + 28), (x + 10, y + 18), (x + 22, y + 22)], fill=e['accent'], outline=e['dark'])
    d.polygon([(x + 38, y + 24), (x + 48, y + 16), (x + 46, y + 28)], fill=e['accent'], outline=e['dark'])
    d.ellipse((x + 38, y + 34, x + 44, y + 40), fill=e['glow'])

    x, y = box(1)
    d.ellipse((x + 16, y + 18, x + 48, y + 46), fill=e['accent'], outline=e['dark'], width=3)
    d.polygon([(x + 20, y + 44), (x + 32, y + 56), (x + 44, y + 44)], fill=e['main'], outline=e['dark'])
    d.ellipse((x + 24, y + 26, x + 30, y + 32), fill=(255, 255, 255, 255))
    d.ellipse((x + 34, y + 26, x + 40, y + 32), fill=(255, 255, 255, 255))

    x, y = box(2)
    rounded(d, (x + 18, y + 18, x + 46, y + 48), 10, fill=e['main'], outline=e['dark'], width=3)
    d.polygon([(x + 18, y + 28), (x + 10, y + 22), (x + 18, y + 18)], fill=e['dark'])
    d.rectangle((x + 44, y + 26, x + 56, y + 32), fill=e['accent'], outline=e['dark'])
    d.rectangle((x + 22, y + 50, x + 28, y + 60), fill=e['dark'])
    d.rectangle((x + 36, y + 50, x + 42, y + 60), fill=e['dark'])

    x, y = box(3)
    rounded(d, (x + 14, y + 14, x + 50, y + 48), 10, fill=e['main'], outline=e['dark'], width=3)
    d.rectangle((x + 8, y + 22, x + 16, y + 48), fill=e['dark'])
    d.rectangle((x + 48, y + 22, x + 56, y + 48), fill=e['dark'])
    d.polygon([(x + 16, y + 18), (x + 32, y + 6), (x + 48, y + 18)], fill=e['accent'], outline=e['dark'])

    x, y = box(4)
    rounded(d, (x + 10, y + 16, x + 54, y + 52), 14, fill=e['main'], outline=e['dark'], width=3)
    d.polygon([(x + 16, y + 20), (x + 8, y + 6), (x + 20, y + 16)], fill=e['accent'], outline=e['dark'])
    d.polygon([(x + 48, y + 20), (x + 56, y + 6), (x + 44, y + 16)], fill=e['accent'], outline=e['dark'])
    d.rectangle((x + 18, y + 32, x + 46, y + 38), fill=e['glow'])

    x, y = box(5)
    rounded(d, (x + 12, y + 14, x + 52, y + 50), 14, fill=e['dark'], outline=e['accent'], width=3)
    for px in [18, 30, 42]:
        d.polygon([(x + px, y + 14), (x + px - 4, y + 2), (x + px + 4, y + 10)], fill=e['glow'], outline=e['accent'])
    d.polygon([(x + 14, y + 44), (x + 32, y + 58), (x + 50, y + 44)], fill=e['main'], outline=e['accent'])

    x, y = box(6)
    d.polygon([(x + 8, y + 36), (x + 22, y + 20), (x + 24, y + 38), (x + 14, y + 46)], fill=e['main'], outline=e['dark'])
    d.polygon([(x + 56, y + 36), (x + 42, y + 20), (x + 40, y + 38), (x + 50, y + 46)], fill=e['main'], outline=e['dark'])
    d.ellipse((x + 22, y + 18, x + 42, y + 38), fill=e['accent'], outline=e['dark'])

    x, y = box(7)
    rounded(d, (x + 18, y + 14, x + 46, y + 50), 10, fill=e['main'], outline=e['dark'], width=3)
    d.rectangle((x + 28, y + 8, x + 36, y + 14), fill=e['glow'])
    d.rectangle((x + 12, y + 28, x + 20, y + 54), fill=e['dark'])
    d.rectangle((x + 44, y + 28, x + 52, y + 54), fill=e['dark'])

    img.save(ASSET_DIR / f'enemy_atlas_{world_name}.png')


def make_weapon_atlas():
    cell = 64
    cols = 6
    rows = 5
    img = Image.new('RGBA', (cell * cols, cell * rows), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    def box(i):
        return (i % cols) * cell, (i // cols) * cell

    def shaft(x, y, hilt):
        d.line((x + 12, y + 48, x + 42, y + 18), fill=hilt, width=5)
        d.line((x + 8, y + 52, x + 16, y + 44), fill=(90, 62, 38, 255), width=6)
        d.line((x + 6, y + 50, x + 18, y + 38), fill=(132, 92, 54, 255), width=2)

    for i, blade in enumerate(WEAPON_COLORS):
        x, y = box(i)
        outline = (40, 40, 44, 255)
        hilt = (128, 92, 54, 255)
        shaft(x, y, hilt)
        kind = i % 6
        if kind == 0:  # sword
            d.polygon([(x + 18, y + 42), (x + 42, y + 18), (x + 48, y + 10), (x + 50, y + 16), (x + 26, y + 40)], fill=blade, outline=outline)
            d.line((x + 20, y + 40, x + 46, y + 14), fill=(255, 255, 255, 180), width=1)
            d.line((x + 12, y + 44, x + 22, y + 34), fill=outline, width=4)
        elif kind == 1:  # axe
            d.line((x + 12, y + 44, x + 22, y + 34), fill=outline, width=4)
            d.polygon([(x + 22, y + 34), (x + 46, y + 28), (x + 50, y + 12), (x + 34, y + 14), (x + 26, y + 20)], fill=blade, outline=outline)
        elif kind == 2:  # spear
            d.line((x + 12, y + 44, x + 20, y + 36), fill=outline, width=4)
            d.line((x + 20, y + 36, x + 48, y + 8), fill=blade, width=4)
            d.polygon([(x + 50, y + 6), (x + 56, y + 10), (x + 48, y + 18), (x + 44, y + 12)], fill=blade, outline=outline)
        elif kind == 3:  # flail / hooked
            d.line((x + 12, y + 44, x + 22, y + 34), fill=outline, width=4)
            d.arc((x + 28, y + 8, x + 58, y + 38), start=220, end=40, fill=blade, width=5)
            d.ellipse((x + 42, y + 8, x + 56, y + 22), fill=blade, outline=outline)
        elif kind == 4:  # hammer
            d.line((x + 12, y + 44, x + 22, y + 34), fill=outline, width=4)
            rounded(d, (x + 28, y + 12, x + 52, y + 24), 4, fill=blade, outline=outline, width=2)
            rounded(d, (x + 30, y + 24, x + 48, y + 30), 3, fill=blade, outline=outline, width=2)
        else:  # halberd / scythe
            d.line((x + 12, y + 44, x + 22, y + 34), fill=outline, width=4)
            d.polygon([(x + 24, y + 26), (x + 48, y + 8), (x + 40, y + 22), (x + 50, y + 28), (x + 28, y + 34)], fill=blade, outline=outline)

        d.rectangle((x + 12, y + 40, x + 20, y + 44), fill=(223, 188, 82, 255), outline=outline)

    img.save(ASSET_DIR / 'weapons_atlas.png')


def make_pet_atlas():
    cell = 64
    img = Image.new('RGBA', (cell * 4, cell), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    x = 0
    d.ellipse((x + 14, 24, x + 46, 48), fill=(214, 118, 62, 255), outline=(112, 58, 34, 255), width=3)
    d.polygon([(x + 18, 24), (x + 10, 12), (x + 22, 18)], fill=(214, 118, 62, 255), outline=(112, 58, 34, 255))
    d.polygon([(x + 42, 24), (x + 50, 12), (x + 38, 18)], fill=(214, 118, 62, 255), outline=(112, 58, 34, 255))
    d.polygon([(x + 44, 40), (x + 58, 32), (x + 50, 48)], fill=(255, 176, 98, 255), outline=(112, 58, 34, 255))

    x = 64
    d.ellipse((x + 18, 16, x + 42, 36), fill=(164, 214, 255, 255), outline=(94, 136, 172, 255), width=3)
    d.polygon([(x + 14, 24), (x + 6, 18), (x + 16, 30)], fill=(132, 182, 236, 255), outline=(94, 136, 172, 255))
    d.polygon([(x + 46, 24), (x + 58, 18), (x + 44, 30)], fill=(132, 182, 236, 255), outline=(94, 136, 172, 255))
    d.polygon([(x + 28, 36), (x + 16, 54), (x + 32, 44), (x + 46, 54), (x + 36, 36)], fill=(206, 236, 255, 255), outline=(94, 136, 172, 255))

    x = 128
    d.ellipse((x + 18, 18, x + 46, 46), fill=(224, 186, 76, 255), outline=(128, 94, 32, 255), width=3)
    d.ellipse((x + 28, 8, x + 38, 18), fill=(255, 220, 118, 255), outline=(128, 94, 32, 255))
    for dx, dy in [(18, 20), (42, 20), (20, 42), (40, 42)]:
        d.ellipse((x + dx - 5, dy - 5, x + dx + 5, dy + 5), fill=(150, 110, 30, 255), outline=(128, 94, 32, 255))

    x = 192
    d.ellipse((x + 14, 20, x + 46, 50), fill=(122, 166, 104, 255), outline=(60, 86, 52, 255), width=3)
    d.ellipse((x + 34, 10, x + 54, 30), fill=(144, 188, 124, 255), outline=(60, 86, 52, 255), width=3)
    d.polygon([(x + 40, 30), (x + 54, 46), (x + 34, 44)], fill=(98, 132, 82, 255), outline=(60, 86, 52, 255))
    d.ellipse((x + 38, 16, x + 42, 20), fill=(255, 255, 255, 255))

    img.save(ASSET_DIR / 'pets_atlas.png')


if __name__ == '__main__':
    for world_name, palette in WORLDS.items():
        make_tile_atlas(world_name, palette)
        make_prop_atlas(world_name, palette)
        make_enemy_atlas(world_name, palette)
    make_actor_atlas()
    make_weapon_atlas()
    make_pet_atlas()
    print('Assets written to', ASSET_DIR)
