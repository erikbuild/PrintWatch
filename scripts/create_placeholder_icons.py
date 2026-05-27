# ABOUTME: Generates placeholder 32x32 silhouette PNGs for printer models.
# ABOUTME: Creates simple geometric outlines recognizable at 1-bit resolution.

from PIL import Image, ImageDraw

SIZE = 64  # draw at 2x for cleaner downscaling


def draw_generic(draw):
    """Generic box-frame FDM printer."""
    # Frame uprights
    draw.rectangle([10, 12, 14, 56], fill=0)
    draw.rectangle([50, 12, 54, 56], fill=0)
    # Top bar
    draw.rectangle([10, 12, 54, 16], fill=0)
    # Bed
    draw.rectangle([6, 48, 58, 52], fill=0)
    # Base
    draw.rectangle([4, 52, 60, 56], fill=0)
    # Nozzle (small triangle)
    draw.polygon([(30, 24), (34, 24), (32, 30)], fill=0)
    # X gantry bar
    draw.rectangle([14, 22, 50, 25], fill=0)


def draw_prusa_i3(draw):
    """Prusa i3-style open frame (MK3/MK4)."""
    # Vertical frame (left side only - cantilever)
    draw.rectangle([12, 8, 16, 56], fill=0)
    # Top bar extending right
    draw.rectangle([12, 8, 52, 12], fill=0)
    # X gantry
    draw.rectangle([16, 26, 48, 29], fill=0)
    # Nozzle
    draw.polygon([(30, 29), (34, 29), (32, 35)], fill=0)
    # Extruder motor block
    draw.rectangle([26, 20, 38, 27], fill=0)
    # Bed
    draw.rectangle([8, 48, 56, 52], fill=0)
    # Y-axis rods
    draw.rectangle([16, 48, 18, 56], fill=0)
    draw.rectangle([46, 48, 48, 56], fill=0)
    # Base
    draw.rectangle([8, 54, 56, 58], fill=0)


def draw_enclosed_cube(draw):
    """Enclosed cube printer (Core One, K1, Bambu)."""
    # Main box
    draw.rectangle([8, 10, 56, 54], outline=0, width=3)
    # Door window
    draw.rectangle([14, 16, 50, 44], outline=0, width=2)
    # Top spool holder
    draw.rectangle([24, 4, 40, 10], fill=0)
    # Base feet
    draw.rectangle([10, 54, 18, 58], fill=0)
    draw.rectangle([46, 54, 54, 58], fill=0)
    # Interior nozzle hint
    draw.polygon([(30, 26), (34, 26), (32, 32)], fill=0)


def draw_voron(draw):
    """Voron-style enclosed CoreXY with visible frame."""
    # Outer frame
    draw.rectangle([6, 8, 58, 56], outline=0, width=2)
    # Inner panel (acrylic window)
    draw.rectangle([12, 14, 52, 50], outline=0, width=1)
    # Top hat / exhaust
    draw.rectangle([20, 4, 44, 8], fill=0)
    # Corner brackets (distinctive Voron look)
    draw.line([(6, 8), (14, 16)], fill=0, width=2)
    draw.line([(58, 8), (50, 16)], fill=0, width=2)
    draw.line([(6, 56), (14, 48)], fill=0, width=2)
    draw.line([(58, 56), (50, 48)], fill=0, width=2)
    # Nozzle hint
    draw.polygon([(30, 28), (34, 28), (32, 34)], fill=0)


def draw_mini(draw):
    """Prusa MINI - smaller, compact i3."""
    # Short vertical frame
    draw.rectangle([16, 14, 20, 52], fill=0)
    # Top bar (shorter reach)
    draw.rectangle([16, 14, 46, 18], fill=0)
    # X gantry
    draw.rectangle([20, 30, 42, 33], fill=0)
    # Nozzle
    draw.polygon([(29, 33), (33, 33), (31, 38)], fill=0)
    # Bed (smaller)
    draw.rectangle([14, 46, 50, 50], fill=0)
    # Base
    draw.rectangle([12, 50, 52, 54], fill=0)


def draw_xl(draw):
    """Prusa XL - large with tool changer."""
    # Wide frame
    draw.rectangle([4, 6, 8, 56], fill=0)
    draw.rectangle([56, 6, 60, 56], fill=0)
    # Top bar
    draw.rectangle([4, 6, 60, 10], fill=0)
    # X gantry
    draw.rectangle([8, 24, 56, 27], fill=0)
    # Tool changer dock (top, multiple slots)
    draw.rectangle([14, 12, 22, 20], fill=0)
    draw.rectangle([24, 12, 32, 20], fill=0)
    draw.rectangle([34, 12, 42, 20], fill=0)
    # Active nozzle
    draw.polygon([(30, 27), (34, 27), (32, 33)], fill=0)
    # Large bed
    draw.rectangle([2, 48, 62, 52], fill=0)
    # Base
    draw.rectangle([4, 52, 60, 56], fill=0)


def draw_ender(draw):
    """Creality Ender 3 - V-slot frame, open."""
    # V-slot uprights
    draw.rectangle([12, 10, 17, 56], fill=0)
    draw.rectangle([47, 10, 52, 56], fill=0)
    # Top bar
    draw.rectangle([12, 10, 52, 15], fill=0)
    # Z lead screw (thin line, right side)
    draw.rectangle([49, 15, 51, 48], fill=0)
    # X gantry
    draw.rectangle([17, 28, 47, 31], fill=0)
    # Bowden extruder (top of frame)
    draw.rectangle([14, 4, 24, 10], fill=0)
    # Nozzle
    draw.polygon([(30, 31), (34, 31), (32, 37)], fill=0)
    # Bed
    draw.rectangle([8, 48, 56, 52], fill=0)
    # Base cross-bar
    draw.rectangle([6, 54, 58, 58], fill=0)


DRAWERS = {
    "generic":  draw_generic,
    "mk4":      draw_prusa_i3,
    "mk3":      draw_prusa_i3,
    "mini":     draw_mini,
    "core_one": draw_enclosed_cube,
    "xl":       draw_xl,
    "v2_4":     draw_voron,
    "trident":  draw_voron,
    "k1":       draw_enclosed_cube,
    "ender":    draw_ender,
    "bambu":    draw_enclosed_cube,
}


def main():
    out_dir = "assets/icons"
    for name, drawer in DRAWERS.items():
        img = Image.new("L", (SIZE, SIZE), 255)
        draw = ImageDraw.Draw(img)
        drawer(draw)
        path = f"{out_dir}/{name}.png"
        img.save(path)
        print(f"  Created {path}")
    print(f"\nGenerated {len(DRAWERS)} placeholder icons in {out_dir}/")


if __name__ == "__main__":
    main()
