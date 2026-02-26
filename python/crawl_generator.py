#!/usr/bin/env python3
"""
Star Wars opening crawl — Comet Busters

The key insight:
  screen_y = H - virtual_y + scroll

  At t=0, scroll=0:
    screen_y = H - virtual_y
    For line to be OFF SCREEN BELOW: screen_y > H  →  virtual_y < 0
    LINES[0] starts at virtual_y = 0 → screen_y = H (bottom edge, just entering)
    LINES[1] starts at virtual_y = -LINE_STEP → screen_y = H + LINE_STEP (below screen)
    ...etc, later lines have more negative virtual_y

  As t increases, scroll = t * SPEED increases:
    screen_y = H - virtual_y + scroll  →  screen_y increases  →  line moves DOWN? No!

  Wait — scrolling UP means screen_y DECREASES over time.
    screen_y = H - virtual_y - scroll

  At t=0: LINES[0] (vy=0) → screen_y = H - 0 - 0 = H  (bottom edge ✓)
  At t=1: LINES[0] (vy=0) → screen_y = H - 0 - SPEED  (moved up ✓)
  
  For LINES[1] to appear AFTER LINES[0]:
    LINES[1] needs screen_y = H at a LATER time
    H = H - vy[1] - scroll  →  vy[1] = -scroll_at_entry  →  vy[1] = -LINE_STEP
    
  So later lines have MORE NEGATIVE virtual_y.
  virtual_y[i] = -i * LINE_STEP
"""
import numpy as np
from PIL import Image, ImageDraw, ImageFont
try:
    from moviepy.editor import VideoClip
except ImportError:
    from moviepy import VideoClip

OUTPUT   = "opening_crawl.mp4"
W, H     = 1920, 1080
FPS      = 30
DURATION = 22.0
FONT     = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"

LINES = [
    "COMET BUSTER",
    "",
    "In the not so distant future",
    "in a galaxy not so far away",
    "",
    "The Kepler-442 Asteroid Field,",
    "once a treasure trove of minerals,",
    "now lies in ruin.",
    "Asteroids fracture, comets drift,",
    "factions clash.",
    "",
    "Red warships hunt without mercy.",
    "Blue patrols guard with fragile honor.",
    "Green drones strip-mine ruthlessly.",
    "And now... the PURPLE SENTINELS arrive—",
    "enigmatic guardians with unknown intent.",
    "",
    "You fly the DESTINY--",
    "an ancient warship of unknown origin,",
    "reborn as a mining vessel,",
    "armed with rapid-fire cannons,",
    "advanced thrusters, drone missiles,",
    "and omnidirectional fire.",
    "",
    "It is fragile, yet fierce.",
    "It carries no banner, no allegiance,",
    "only the will to survive.",
    "",
    "But survival is not enough.",
    "Beyond the factions loom colossal threats:",
    "MEGA BOSS SHIPS, engines of annihilation,",
    "whose presence darkens the field itself.",
    "",
    "And deeper still, from the void,",
    "alien forces gather—",
    "a tide that consumes all in its path.",
    "",
    "Your mission: endure the chaos,",
    "outwit rival factions,",
    "and face the horrors that await.",
    "",
    "The asteroid field is no longer a mine.",
    "It is a crucible of war.",
    "",
    "Survive. Score. Ascend.",
]

BODY_PX    = 72
TITLE_MUL  = 1.8
LINE_STEP  = 95
BLANK_STEP = 60
HORIZON_Y  = -300        # vanishing point is above/off the top of the screen
SCROLL_SPD = 220

# virtual_y: 0 for first line, negative for subsequent lines
# screen_y = H - virtual_y - scroll
# line enters bottom (screen_y=H) when scroll = -virtual_y
line_vy = []
vy = 0
for line in LINES:
    line_vy.append(vy)
    vy -= (BLANK_STEP if line == "" else LINE_STEP)  # SUBTRACT so later lines enter later

TOTAL = abs(vy)
print(f"Lines: {len(LINES)}, span: {TOTAL}px, last enters at t={TOTAL/SCROLL_SPD:.1f}s")

# Starfield
rng = np.random.default_rng(42)
_sx = rng.integers(0, W, 700); _sy = rng.integers(0, H, 700)
_sb = rng.integers(60, 240, 700); _ss = rng.choice([1,1,2,2,3], 700)
STARS = np.zeros((H, W, 3), dtype=np.uint8)
for x, y_, b, s in zip(_sx, _sy, _sb, _ss):
    STARS[y_:y_+s, x:x+s] = b

_fonts = {}
def fnt(sz):
    sz = max(6, int(sz))
    if sz not in _fonts:
        try: _fonts[sz] = ImageFont.truetype(FONT, sz)
        except: _fonts[sz] = ImageFont.load_default()
    return _fonts[sz]

_mi = Image.new("RGBA",(1,1)); _md = ImageDraw.Draw(_mi)
def tsz(text, f):
    bb = _md.textbbox((0,0), text, font=f)
    return bb[2]-bb[0], bb[3]-bb[1]

def make_frame(t):
    scroll = t * SCROLL_SPD
    canvas = Image.fromarray(STARS.copy())
    draw   = ImageDraw.Draw(canvas)

    for i, line in enumerate(LINES):
        if line == "": continue

        screen_y = H - line_vy[i] - scroll   # baseline y on screen

        # Cull lines that are far off-screen bottom or well above screen top
        if screen_y > H + 150 or screen_y < -200:
            continue

        scale = (screen_y - HORIZON_Y) / float(H - HORIZON_Y)
        scale = max(0.15, min(1.0, scale))  # floor raised: text never shrinks below 15%
                                             # so no invisible gap as lines approach top

        is_title = (line == "COMET BUSTER")
        fsz = BODY_PX * (TITLE_MUL if is_title else 1.0) * scale
        f   = fnt(fsz)
        tw, th = tsz(line, f)
        x   = (W - tw) // 2
        y_  = int(screen_y) - th

        # Fade in only near the bottom; lines at top stay fully opaque until they leave
        fade  = min(1.0, (scale - 0.15) / 0.08)
        alpha = int(255 * fade)

        if alpha <= 0:
            continue

        for dx, dy in [(-2,0),(2,0),(0,-2),(0,2)]:
            draw.text((x+dx,y_+dy), line, font=f, fill=(255,165,0,int(alpha*0.4)))
        draw.text((x, y_), line, font=f, fill=(255,220,50,alpha))

    result = np.array(canvas)

    # Fade vignette only within the visible screen (HORIZON_Y is off-screen so clamp to 0)
    fade_top = max(0, HORIZON_Y)
    vh = int((H - fade_top) * 0.08)   # narrow fade band at top of screen
    for row in range(fade_top, fade_top + vh):
        result[row] = (result[row] * (row - fade_top) / vh).astype(np.uint8)

    return result

# Verify ordering
print("Verification at t=0 (all should be OFF-SCREEN, screen_y >= H):")
for i in range(min(5, len(LINES))):
    sy = H - line_vy[i] - 0
    status = "✓ off-screen" if sy >= H else f"✗ VISIBLE at {sy}"
    print(f"  LINES[{i}] '{LINES[i][:20]}' vy={line_vy[i]} screen_y={sy} {status}")

print(f"\nRendering {DURATION}s @ {FPS}fps …")
VideoClip(make_frame, duration=DURATION).write_videofile(
    OUTPUT, fps=FPS, codec="libx264", audio=False, logger="bar")
print(f"✓ {OUTPUT}")
