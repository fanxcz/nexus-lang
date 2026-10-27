"""Regenerate the NEXUS Poker 2.1.0 art and sound assets.

Run from the repository root:

    python examples/assets/generate_assets.py

Requires Pillow (only for the card textures). The WAV files use the standard
library ``wave`` module. All output is deterministic so regenerating the pack
does not produce spurious diffs.
"""

import math
import os
import random
import struct
import wave

from PIL import Image, ImageDraw, ImageFilter, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
CARD_DIR = os.path.join(HERE, "cards")
SOUND_DIR = os.path.join(HERE, "sounds")
TABLE_DIR = os.path.join(HERE, "table")
RATE = 22050

TABLE_W, TABLE_H = 1920, 1080
TABLE_SCALE = 2
TCX, TCY = 960, 520

RANKS = ["2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"]
SUITS = [("s", "black"), ("h", "red"), ("d", "red"), ("c", "black")]
SYMBOL = {"s": "\u2660", "h": "\u2665", "d": "\u2666", "c": "\u2663"}

CARD_W, CARD_H = 144, 208


def _fonts():
    path = "C:/Windows/Fonts/arialbd.ttf"
    try:
        return (
            ImageFont.truetype(path, 40),
            ImageFont.truetype(path, 64),
        )
    except OSError:
        fallback = ImageFont.load_default()
        return fallback, fallback


def make_card(rank, suit, color, rank_font, suit_font):
    img = Image.new("RGBA", (CARD_W, CARD_H), (255, 255, 255, 255))
    draw = ImageDraw.Draw(img)
    draw.rounded_rectangle(
        [0, 0, CARD_W - 1, CARD_H - 1], radius=12, outline=(0, 0, 0, 255),
        width=3, fill=(255, 255, 255, 255),
    )
    draw.rounded_rectangle(
        [4, 4, CARD_W - 5, CARD_H - 5], radius=9, outline=(200, 200, 200, 255), width=1
    )
    draw.text((10, 2), rank, font=rank_font, fill=color)
    draw.text((CARD_W // 2 - 18, CARD_H // 2 - 34), SYMBOL[suit], font=suit_font, fill=color)
    rank_width = draw.textlength(rank, font=rank_font)
    draw.text((CARD_W - rank_width - 10, CARD_H - 52), rank, font=rank_font, fill=color)
    return img


def make_back():
    """Restrained graphite-and-gold card back matching the NEXUS identity."""
    img = Image.new("RGBA", (CARD_W, CARD_H), (8, 9, 10, 255))
    draw = ImageDraw.Draw(img)
    draw.rounded_rectangle(
        [1, 1, CARD_W - 2, CARD_H - 2],
        radius=12,
        fill=(9, 10, 11, 255),
        outline=(245, 197, 24, 255),
        width=3,
    )
    draw.rounded_rectangle(
        [7, 7, CARD_W - 8, CARD_H - 8],
        radius=8,
        fill=(12, 13, 14, 255),
        outline=(99, 82, 24, 255),
        width=1,
    )
    # Fine diagonal pinstripes are intentionally subtle behind the central mark.
    for k in range(-CARD_H, CARD_W, 18):
        x0 = max(12, k)
        y0 = max(12, -k)
        x1 = min(CARD_W - 13, k + CARD_H - 24)
        y1 = min(CARD_H - 13, CARD_H - 24 - k)
        if x0 < x1 and y0 < y1:
            draw.line((x0, y0, x1, y1), fill=(42, 38, 21, 255), width=1)
    draw.rounded_rectangle(
        [23, 48, CARD_W - 24, CARD_H - 49],
        radius=7,
        fill=(9, 10, 11, 255),
        outline=(181, 143, 24, 255),
        width=2,
    )
    draw.rounded_rectangle(
        [28, 53, CARD_W - 29, CARD_H - 54],
        radius=5,
        outline=(54, 48, 27, 255),
        width=1,
    )
    # Angular N, using only polygons so the source font does not affect the mark.
    draw.polygon(
        [(43, 76), (51, 76), (91, 124), (91, 78), (101, 78),
         (101, 137), (93, 137), (53, 91), (53, 135), (43, 135)],
        fill=(245, 197, 24, 255),
    )
    for x, y in ((17, 17), (CARD_W - 18, 17),
                 (17, CARD_H - 18), (CARD_W - 18, CARD_H - 18)):
        draw.rectangle((x - 1, y - 1, x + 1, y + 1), fill=(245, 197, 24, 255))
    return img


def write_wav(name, samples):
    path = os.path.join(SOUND_DIR, name)
    with wave.open(path, "w") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(RATE)
        frames = b"".join(
            struct.pack("<h", max(-32767, min(32767, int(sample * 32767))))
            for sample in samples
        )
        handle.writeframes(frames)


def tone(freq, duration, volume=0.35):
    count = int(RATE * duration)
    return [
        math.sin(2 * math.pi * freq * i / RATE) * volume * (1.0 - i / count)
        for i in range(count)
    ]


def sequence(notes, duration, volume=0.3):
    samples = []
    for freq in notes:
        samples.extend(tone(freq, duration, volume))
    return samples


def _table_fonts():
    try:
        return ImageFont.truetype("C:/Windows/Fonts/arialbd.ttf", 104 * TABLE_SCALE)
    except OSError:
        return ImageFont.load_default()


def make_table():
    """Draws the whole oval table once so the game can blit a single texture."""
    img = Image.new("RGBA", (TABLE_W * TABLE_SCALE, TABLE_H * TABLE_SCALE), (0, 0, 0, 0))

    def ellipse(draw, cx, cy, rx, ry, **kwargs):
        draw.ellipse(
            [
                (cx - rx) * TABLE_SCALE,
                (cy - ry) * TABLE_SCALE,
                (cx + rx) * TABLE_SCALE,
                (cy + ry) * TABLE_SCALE,
            ],
            **kwargs
        )

    # Soft drop shadow under the rail.
    shadow = Image.new("RGBA", img.size, (0, 0, 0, 0))
    ellipse(ImageDraw.Draw(shadow), TCX, TCY + 18, 862, 400, fill=(0, 0, 0, 160))
    shadow = shadow.filter(ImageFilter.GaussianBlur(22 * TABLE_SCALE))
    img = Image.alpha_composite(img, shadow)
    draw = ImageDraw.Draw(img)

    # Wooden rail with a few grain rings.
    ellipse(draw, TCX, TCY, 852, 392, fill=(46, 24, 9, 255))
    for i in range(16):
        t = i / 15.0
        tone = int(58 - 16 * t)
        ellipse(
            draw, TCX, TCY, 850 - 3.2 * i, 390 - 3.2 * i,
            outline=(tone + 18, tone, tone // 2, 255), width=1,
        )

    # Beveled gold rim.
    for i in range(12):
        t = i / 11.0
        k = abs(t - 0.5) * 2.0
        ellipse(
            draw, TCX, TCY, 818 - 2.0 * i, 364 - 2.0 * i,
            fill=(int(120 + 84 * (1 - k)), int(88 + 62 * (1 - k)),
                  int(26 + 32 * (1 - k)), 255),
        )

    # Inner rail and felt.
    ellipse(draw, TCX, TCY, 800, 352, fill=(52, 28, 10, 255))
    ellipse(draw, TCX, TCY, 792, 346, fill=(24, 12, 5, 255))
    ellipse(draw, TCX, TCY, 784, 340, fill=(6, 56, 24, 255))
    ellipse(draw, TCX, TCY, 770, 328, fill=(8, 70, 30, 255))
    ellipse(draw, TCX, TCY, 756, 316, fill=(10, 82, 36, 255))

    # Soft inner shadow around the felt edge.
    edge = Image.new("RGBA", img.size, (0, 0, 0, 0))
    ellipse(ImageDraw.Draw(edge), TCX, TCY, 756, 316, outline=(0, 0, 0, 150), width=30)
    edge = edge.filter(ImageFilter.GaussianBlur(14 * TABLE_SCALE))
    img = Image.alpha_composite(img, edge)

    # Deterministic felt speckles.
    speckles = Image.new("RGBA", img.size, (0, 0, 0, 0))
    sd = ImageDraw.Draw(speckles)
    rng = random.Random(20261026)
    for _ in range(5200):
        a = rng.uniform(0.0, math.tau)
        r = math.sqrt(rng.random())
        x = TCX + 744 * r * math.cos(a)
        y = TCY + 308 * r * math.sin(a)
        size = rng.uniform(0.8, 2.4) * TABLE_SCALE
        if rng.random() < 0.55:
            col = (0, 44, 18, rng.randint(12, 30))
        else:
            col = (60, 150, 80, rng.randint(10, 24))
        sd.ellipse([x * TABLE_SCALE - size, y * TABLE_SCALE - size,
                    x * TABLE_SCALE + size, y * TABLE_SCALE + size], fill=col)
    img = Image.alpha_composite(img, speckles)

    # Gold stitch dots just inside the felt.
    draw = ImageDraw.Draw(img)
    for k in range(72):
        a = math.tau * k / 72.0
        x = TCX + 788 * math.cos(a)
        y = TCY + 344 * math.sin(a)
        r = 3.0 * TABLE_SCALE
        draw.ellipse([x * TABLE_SCALE - r, y * TABLE_SCALE - r,
                      x * TABLE_SCALE + r, y * TABLE_SCALE + r],
                     fill=(196, 162, 84, 110))

    # Faint centre watermark, mostly covered by the board.
    font = _table_fonts()
    try:
        mark = Image.new("RGBA", img.size, (0, 0, 0, 0))
        md = ImageDraw.Draw(mark)
        box = md.textbbox((0, 0), "NEXUS", font=font)
        tw = box[2] - box[0]
        th = box[3] - box[1]
        md.text(((TCX * TABLE_SCALE) - tw / 2.0,
                 (TCY * TABLE_SCALE) - th / 2.0 - box[1]),
                "NEXUS", font=font, fill=(220, 235, 220, 26))
        img = Image.alpha_composite(img, mark)
    except Exception:
        pass

    return img.resize((TABLE_W, TABLE_H), Image.LANCZOS)


def generate_table():
    os.makedirs(TABLE_DIR, exist_ok=True)
    make_table().save(os.path.join(TABLE_DIR, "felt.png"))


UI_DIR = os.path.join(HERE, "ui")


def make_cursor():
    """Crisp spade cursor with a fixed tip hotspot, cyan keyline and solid fill."""
    scale = 4
    w, h = 60 * scale, 78 * scale
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    points = [(1, 1), (1, 58), (15, 45), (25, 72), (36, 67), (26, 40), (46, 40)]
    pts = [(x * scale, y * scale) for x, y in points]
    shadow = [(x + 2 * scale, y + 3 * scale) for x, y in pts]
    draw.polygon(shadow, fill=(0, 0, 0, 115))

    # A cyan edge separates the white cursor from both dark and green areas.
    draw.line(pts + [pts[0]], fill=(36, 165, 245, 255), width=5 * scale, joint="curve")
    draw.polygon(pts, fill=(245, 247, 250, 255))
    draw.line(pts + [pts[0]], fill=(10, 17, 28, 255), width=2 * scale, joint="curve")
    draw.line(
        [(3 * scale, 5 * scale), (3 * scale, 47 * scale)],
        fill=(255, 255, 255, 255),
        width=1 * scale,
    )

    # High-contrast spade stamp inside the arrow body.
    spade = [(18, 20), (13, 25), (10, 28), (11, 31), (14, 32),
             (18, 29), (22, 32), (25, 31), (26, 28), (23, 25)]
    draw.polygon([(x * scale, y * scale) for x, y in spade],
                 fill=(12, 20, 31, 255))
    draw.line([(18 * scale, 29 * scale), (18 * scale, 35 * scale)],
              fill=(12, 20, 31, 255), width=2 * scale)
    draw.polygon([(15 * scale, 35 * scale), (21 * scale, 35 * scale),
                  (19 * scale, 32 * scale), (17 * scale, 32 * scale)],
                 fill=(12, 20, 31, 255))
    return img.resize((60, 78), Image.LANCZOS)


def generate_cursor():
    os.makedirs(UI_DIR, exist_ok=True)
    make_cursor().save(os.path.join(UI_DIR, "cursor.png"))


def generate_cards():
    os.makedirs(CARD_DIR, exist_ok=True)
    rank_font, suit_font = _fonts()
    for rank_index, rank in enumerate(RANKS):
        for suit_index, (suit, colour_name) in enumerate(SUITS):
            colour = (200, 20, 20, 255) if colour_name == "red" else (25, 25, 25, 255)
            make_card(rank, suit, colour, rank_font, suit_font).save(
                os.path.join(CARD_DIR, "card_{}.png".format(rank_index * 4 + suit_index))
            )
    make_back().save(os.path.join(CARD_DIR, "card_back.png"))


def generate_sounds():
    os.makedirs(SOUND_DIR, exist_ok=True)

    deal = []
    for i in range(int(RATE * 0.07)):
        t = i / RATE
        env = 1.0 - i / (RATE * 0.07)
        deal.append(math.sin(2 * math.pi * (700 + 900 * t) * t) * 0.25 * env)
    write_wav("deal.wav", deal)
    write_wav("chip.wav", sequence([1250, 1600], 0.045, 0.28))
    write_wav("win.wav", sequence([523, 659, 784, 1047], 0.10, 0.3))
    write_wav("lose.wav", sequence([392, 330, 262, 196], 0.11, 0.28))
    write_wav("fold.wav", sequence([180, 140], 0.09, 0.3))


if __name__ == "__main__":
    generate_cards()
    generate_table()
    generate_cursor()
    generate_sounds()
    print("Regenerated card textures in", CARD_DIR)
    print("Regenerated table texture in", TABLE_DIR)
    print("Regenerated cursor texture in", UI_DIR)
    print("Regenerated sounds in", SOUND_DIR)
