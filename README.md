# Arrows GO — Nintendo 3DS

Arrows GO is a puzzle game for Nintendo 3DS where arrows are arranged on a grid, each pointing in a direction. The goal is to remove all arrows by launching them off the board. An arrow can only escape if its entire path to the edge is completely clear. Tap the wrong arrow and you lose a heart. Plan the order, clear the board.

**Notice:** It is currently in the development phase.

---

## Controls

| Input | Action |
|---|---|
| Touch Screen | Tap an arrow to launch it |
| D-Pad | Move cursor between cells |
| A | Launch the selected arrow |
| Y | Show hint (highlights a safe arrow) |
| L / R | Zoom out / Zoom in |
| Circle Pad | Pan camera when zoomed |
| SELECT | Restart current level |
| START | Pause / Resume |

---

## How to Play

1. Look at the grid and find an arrow whose path to the edge is completely empty.
2. Tap it (or press A) — it flies off the board.
3. Each arrow you remove opens new paths for others.
4. Clear all arrows to complete the level.
5. Tapping a blocked arrow costs one heart. You have 3 hearts per level.
6. Use the hint (Y) if you get stuck — it highlights one valid arrow.

---

## Building

Requirements: [devkitPro](https://devkitpro.org/) with 3DS support and citro2d installed.

```bash
cd "Arrows GO"
make
```

Output: `ArrowsGO.3dsx`

> **Note:** The folder name must not contain spaces. Rename to `ArrowsGO` or `Arrows_GO` before running `make`.

---

## Installing on 3DS

1. Copy `ArrowsGO.3dsx` to your SD card under `/3ds/ArrowsGO/`
2. Launch via **Homebrew Launcher** on your 3DS

---

## Levels

The game includes 8 levels of increasing difficulty:

| Level | Grid | Arrows |
|---|---|---|
| 1 | 4×4 | 5 |
| 2 | 4×4 | 7 |
| 3 | 5×5 | 9 |
| 4 | 5×5 | 10 |
| 5 | 6×6 | 12 |
| 6 | 6×6 | 14 |
| 7 | 7×7 | 16 |
| 8 | 7×7 | 18 |

The grid auto-fits to the screen on each level load. Use L/R and the Circle Pad to zoom and pan on larger grids.

---

## Project Structure

```
Arrows GO/
├── Makefile
├── README.md
└── source/
    └── main.cpp
```

---

## Built With

- [devkitARM](https://devkitpro.org/) — ARM toolchain for Nintendo 3DS
- [libctru](https://github.com/devkitPro/libctru) — 3DS homebrew library
- [citro2d](https://github.com/devkitPro/citro2d) — 2D rendering library
