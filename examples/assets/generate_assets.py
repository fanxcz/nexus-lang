"""Regenerate the NEXUS Poker art and sound assets.

Run from the repository root:

    python examples/assets/generate_assets.py

Requires Pillow (only for the card textures). The WAV files use the standard
library ``wave`` module. All output is deterministic so regenerating the pack
does not produce spurious diffs.
"""

import math
import os
import struct
import wave

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
CARD_DIR = os.path.join(HERE, "cards")
SOUND_DIR = os.path.join(HERE, "sounds")
RATE = 22050

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
    img = Image.new("RGBA", (CARD_W, CARD_H), (20, 20, 70, 255))
    draw = ImageDraw.Draw(img)
    draw.rounded_rectangle(
        [0, 0, CARD_W - 1, CARD_H - 1], radius=12, outline=(230, 230, 230, 255),
        width=3, fill=(24, 30, 110, 255),
    )
    for y in range(10, CARD_H - 10, 14):
        for x in range(10, CARD_W - 10, 14):
            draw.rectangle([x, y, x + 6, y + 6], fill=(50, 70, 180, 255))
    draw.rounded_rectangle(
        [16, 16, CARD_W - 17, CARD_H - 17], radius=8, outline=(120, 140, 220, 255), width=2
    )
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
    generate_sounds()
    print("Regenerated card textures in", CARD_DIR)
    print("Regenerated sounds in", SOUND_DIR)
