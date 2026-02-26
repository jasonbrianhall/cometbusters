#!/usr/bin/env python3
"""
Star Wars opening crawl — Comet Busters
Supersampled (2×) for smooth subpixel scroll.
Per-segment color with colored glow.

To render:
    python -c "
    from crawl_generator import make_frame, DURATION, FPS, OUTPUT
    try:
        from moviepy.editor import VideoClip
    except ImportError:
        from moviepy import VideoClip
    VideoClip(make_frame, duration=DURATION).write_videofile(OUTPUT, fps=FPS, codec='libx264', audio=False)
    "
"""
import numpy as np
from PIL import Image, ImageDraw, ImageFont
try:
    from moviepy.editor import VideoClip
except ImportError:
    from moviepy import VideoClip

OUTPUT   = "opening_crawl.mp4"
W, H     = 1920, 1080
SS       = 2            # supersampling: render 2× then Lanczos-downscale
WW, HH   = W * SS, H * SS
FPS      = 30
DURATION = 22.0
FONT     = "/usr/share/fonts/truetype/crosextra/Carlito-Bold.ttf"

# ── Palette ──────────────────────────────────────────────────────────────────
GOLD    = (255, 215,  40)   # classic Star-Wars crawl gold
WHITE   = (255, 255, 255)
DIM     = (160, 160, 170)   # "filler" words — cool grey
RED     = (255,  50,  50)
CRIMSON = (220,  20,  60)
ORANGE  = (255, 140,  20)
AMBER   = (255, 190,  40)
BLUE    = ( 70, 160, 255)
CYAN    = ( 40, 230, 255)
GREEN   = ( 60, 255, 110)
LIME    = (170, 255,  50)
PURPLE  = (200,  70, 255)
VIOLET  = (160,  80, 255)

# ── Script ───────────────────────────────────────────────────────────────────
# Each entry is a list of (word_or_phrase, color) tuples → rendered on one line
# None = blank gap line
LINES = [
    # ── TITLE ─────────────────────────────────────────────────
    [("COMET", CYAN), ("BUSTER", ORANGE)],
    None,

    # ── ACT I: Setting ────────────────────────────────────────
    [("In the not so distant", DIM), ("future", WHITE)],
    [("in a galaxy", DIM), ("not so far away", GOLD)],
    None,
    [("The", DIM), ("Kepler-442", CYAN), ("Asteroid Field,", GOLD)],
    [("once a treasure trove of minerals,", DIM)],
    [("now lies", WHITE), ("in", DIM), ("ruin.", CRIMSON)],
    [("Asteroids", ORANGE), ("fracture,", DIM), ("comets", CYAN), ("drift,", DIM)],
    [("factions", WHITE), ("clash.", CRIMSON)],
    None,

    # ── ACT II: The Factions ──────────────────────────────────
    [("RED", RED), ("warships", DIM), ("hunt without", DIM), ("mercy.", CRIMSON)],
    [("BLUE", BLUE), ("patrols guard with fragile", DIM), ("honor.", BLUE)],
    [("GREEN", GREEN), ("drones", DIM), ("strip-mine", LIME), ("ruthlessly.", GREEN)],
    [("And now... the", DIM), ("PURPLE SENTINELS", PURPLE), ("arrive—", WHITE)],
    [("enigmatic guardians with", DIM), ("unknown intent.", VIOLET)],
    None,

    # ── ACT III: The Ship ─────────────────────────────────────
    [("You fly the", DIM), ("DESTINY", CYAN), ("--", DIM)],
    [("an ancient warship of", DIM), ("unknown origin,", GOLD)],
    [("reborn as a", DIM), ("mining vessel,", AMBER)],
    [("armed with", DIM), ("rapid-fire cannons,", ORANGE)],
    [("advanced thrusters,", DIM), ("drone missiles,", ORANGE)],
    [("and", DIM), ("omnidirectional fire.", RED)],
    None,
    [("It is", DIM), ("fragile,", CRIMSON), ("yet", DIM), ("fierce.", ORANGE)],
    [("It carries no banner,", DIM), ("no allegiance,", CRIMSON)],
    [("only the will to", DIM), ("survive.", WHITE)],
    None,

    # ── ACT IV: The Threat ────────────────────────────────────
    [("But survival is", DIM), ("not", CRIMSON), ("enough.", WHITE)],
    [("Beyond the factions loom", DIM), ("colossal threats:", ORANGE)],
    [("MEGA BOSS SHIPS,", RED), ("engines of", DIM), ("annihilation,", CRIMSON)],
    [("whose presence", DIM), ("darkens", CRIMSON), ("the field itself.", DIM)],
    None,
    [("And deeper still,", DIM), ("from the", DIM), ("void,", PURPLE)],
    [("alien forces", VIOLET), ("gather—", WHITE)],
    [("a tide that", DIM), ("consumes all", CRIMSON), ("in its path.", DIM)],
    None,

    # ── ACT V: The Call ───────────────────────────────────────
    [("Your mission:", WHITE), ("endure the chaos,", ORANGE)],
    [("outwit rival factions,", DIM)],
    [("and face the", DIM), ("horrors", CRIMSON), ("that await.", DIM)],
    None,
    [("The asteroid field is", DIM), ("no longer a mine.", GOLD)],
    [("It is a", DIM), ("crucible", CRIMSON), ("of", DIM), ("war.", RED)],
    None,

    # ── FINALE ────────────────────────────────────────────────
    [("Survive.", RED), ("  Score.", GOLD), ("  Ascend.", CYAN)],
]

# ── Layout constants ──────────────────────────────────────────────────────────
BODY_PX    = 72
TITLE_MUL  = 1.9
LINE_STEP  = 95
BLANK_STEP = 60
HORIZON_Y  = -800 * SS
SCROLL_SPD = 220

# ── Precompute virtual-y per line ─────────────────────────────────────────────
line_vy = []
vy = 0
for line in LINES:
    line_vy.append(vy)
    vy -= BLANK_STEP if line is None else LINE_STEP

TOTAL = abs(vy)
print(f"Lines: {len(LINES)}, span: {TOTAL}px, last enters at t={TOTAL/SCROLL_SPD:.1f}s")

# ── Starfield ─────────────────────────────────────────────────────────────────
rng = np.random.default_rng(42)
_sx = rng.integers(0, WW, 700)
_sy = rng.integers(0, HH, 700)
_sb = rng.integers(60, 240, 700)
_ss = rng.choice([1, 1, 2, 2, 3], 700)
STARS = np.zeros((HH, WW, 3), dtype=np.uint8)
for x, y_, b, s in zip(_sx, _sy, _sb, _ss):
    STARS[y_:y_+s, x:x+s] = b

# ── Font cache ────────────────────────────────────────────────────────────────
_fonts = {}
def fnt(sz):
    sz = max(6, int(sz))
    if sz not in _fonts:
        try:
            _fonts[sz] = ImageFont.truetype(FONT, sz)
        except Exception:
            _fonts[sz] = ImageFont.load_default()
    return _fonts[sz]

_mi = Image.new("RGBA", (1, 1))
_md = ImageDraw.Draw(_mi)
def tsz(text, f):
    bb = _md.textbbox((0, 0), text, font=f)
    return bb[2] - bb[0], bb[3] - bb[1]

# ── Frame renderer ────────────────────────────────────────────────────────────
def make_frame(t):
    scroll = t * SCROLL_SPD * SS
    canvas = Image.fromarray(STARS.copy())
    draw   = ImageDraw.Draw(canvas)

    for i, line in enumerate(LINES):
        if line is None:
            continue

        screen_y = HH - line_vy[i] * SS - scroll

        # Cull well off-screen
        if screen_y > HH + 150 * SS or screen_y < -200 * SS:
            continue

        # Perspective scale (1.0 at bottom, shrinks toward horizon)
        scale = (screen_y - HORIZON_Y) / float(HH - HORIZON_Y)
        scale = max(0.15, min(1.0, scale))

        is_title = (i == 0)
        fsz = BODY_PX * SS * (TITLE_MUL if is_title else 1.0) * scale
        f   = fnt(fsz)

        # Fade in near the bottom edge
        fade  = min(1.0, (scale - 0.15) / 0.08)
        alpha = int(255 * fade)
        if alpha <= 0:
            continue

        # ── Measure line for centering ────────────────────────
        space_w, _ = tsz(" ", f)
        total_w = sum(tsz(s[0], f)[0] for s in line) + space_w * (len(line) - 1)
        _, th   = tsz(line[0][0], f)

        x_cursor = (WW - total_w) / 2
        y_       = screen_y - th

        # ── Draw each segment ─────────────────────────────────
        for seg_text, seg_color in line:
            sw, _ = tsz(seg_text, f)

            # Soft wide glow — reduced radius to avoid bleeding into narrow glyphs
            glow_alpha = int(alpha * 0.18)
            glow_fill  = (*seg_color, glow_alpha)
            for dx, dy in [(-3, 0), (3, 0), (0, -3), (0, 3),
                           (-2, -2), (2, 2), (-2, 2), (2, -2)]:
                draw.text((x_cursor + dx, y_ + dy), seg_text, font=f, fill=glow_fill)

            # Tighter inner glow
            inner_alpha = int(alpha * 0.30)
            inner_fill  = (*seg_color, inner_alpha)
            for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                draw.text((x_cursor + dx, y_ + dy), seg_text, font=f, fill=inner_fill)

            # Main text
            draw.text((x_cursor, y_), seg_text, font=f, fill=(*seg_color, alpha))

            x_cursor += sw + space_w

    # ── Downscale 2× → 1× (Lanczos smooths the scroll) ───────────────────────
    canvas = canvas.resize((W, H), Image.LANCZOS)
    result = np.array(canvas)

    # ── Vignette: fade top ~8% to black ───────────────────────────────────────
    vh = int(H * 0.08)
    for row in range(vh):
        result[row] = (result[row] * row / vh).astype(np.uint8)

    return result


# ── Run ───────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    print("\nVerification at t=0 (first 6 lines):")
    for i in range(min(6, len(LINES))):
        sy    = H - line_vy[i]
        label = "—blank—" if LINES[i] is None else " ".join(s[0] for s in LINES[i])[:40]
        ok    = "✓ off-screen" if sy >= H else f"✗ VISIBLE at y={sy}"
        print(f"  [{i:02d}] vy={line_vy[i]:5d}  screen_y={sy:5d}  {ok}  \"{label}\"")

    print(f"\nRendering {DURATION}s @ {FPS}fps → {OUTPUT} …")
    VideoClip(make_frame, duration=DURATION).write_videofile(
        OUTPUT, fps=FPS, codec="libx264", audio=False, logger="bar")
    print(f"✓ Done: {OUTPUT}")
