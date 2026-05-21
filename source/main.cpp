/*
    Arrows GO! for Nintendo 3DS
    Puzzle game: tap arrows with a free path to the edge to launch them.
    Clear all arrows to win the level.

    Controls:
      Touch Screen  - Tap an arrow to launch it
      D-Pad         - Move cursor between arrows
      A             - Launch selected arrow (cursor)
      L / R         - Zoom out / Zoom in
      Circle Pad    - Pan camera (when zoomed)
      Y             - Show hint (highlights a safe arrow)
      START         - Pause / Resume
      SELECT        - Restart current level
*/

#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

//=============================================================================
// Constants
//=============================================================================
#define TOP_W           400
#define TOP_H           240
#define BOT_W           320
#define BOT_H           240

#define MAX_ROWS        8
#define MAX_COLS        8

#define CELL_BASE       38      // base cell size (pixels) at zoom=1.0
#define FLY_SPEED       0.08f   // fraction of off-screen per frame
#define ZOOM_STEP       0.20f
#define ZOOM_MIN        0.50f
#define ZOOM_MAX        2.00f
#define CAM_SPEED       3.5f
#define CPAD_DEADZONE   40

#define FLASH_FRAMES    18
#define NUM_LEVELS      8
#define MAX_HEARTS      3

//=============================================================================
// Types
//=============================================================================
typedef enum { DIR_RIGHT=0, DIR_LEFT=1, DIR_UP=2, DIR_DOWN=3 } Direction;
typedef enum {
    GS_PLAYING,
    GS_FLASH,       // wrong-move flash (still playing)
    GS_FLYING,      // arrow flying off (input locked)
    GS_CLEAR,       // level cleared, waiting for input
    GS_OVER,        // game over, waiting for input
    GS_PAUSED
} GameStatus;

struct Cell {
    bool      active;
    Direction dir;
};

struct LevelDef {
    int rows, cols;
    int count;
    struct { int r, c; Direction d; } arrows[32];
};

//=============================================================================
// Level data
//=============================================================================
static const LevelDef LEVELS[NUM_LEVELS] = {
    // 1 — 4x4, 4 arrows (very easy)
    { 4, 4, 4, {
        {0,0,DIR_RIGHT}, {1,3,DIR_LEFT},
        {2,1,DIR_DOWN},  {3,2,DIR_UP}
    }},
    // 2 — 4x4, 6 arrows
    { 4, 4, 6, {
        {0,1,DIR_UP},    {0,3,DIR_RIGHT},
        {1,0,DIR_LEFT},  {2,2,DIR_DOWN},
        {3,0,DIR_RIGHT}, {3,3,DIR_UP}
    }},
    // 3 — 5x5, 8 arrows
    { 5, 5, 8, {
        {0,2,DIR_UP},    {1,0,DIR_LEFT},
        {1,4,DIR_RIGHT}, {2,1,DIR_DOWN},
        {2,3,DIR_UP},    {3,2,DIR_RIGHT},
        {4,0,DIR_DOWN},  {4,4,DIR_LEFT}
    }},
    // 4 — 5x5, 10 arrows
    { 5, 5, 10, {
        {0,0,DIR_UP},    {0,4,DIR_RIGHT},
        {1,2,DIR_LEFT},  {2,0,DIR_DOWN},
        {2,4,DIR_UP},    {3,1,DIR_RIGHT},
        {3,3,DIR_LEFT},  {4,0,DIR_RIGHT},
        {4,2,DIR_DOWN},  {4,4,DIR_LEFT}
    }},
    // 5 — 6x6, 12 arrows
    { 6, 6, 12, {
        {0,1,DIR_UP},    {0,4,DIR_RIGHT},
        {1,5,DIR_RIGHT}, {2,0,DIR_LEFT},
        {2,3,DIR_DOWN},  {3,2,DIR_UP},
        {3,5,DIR_DOWN},  {4,1,DIR_LEFT},
        {4,4,DIR_RIGHT}, {5,0,DIR_DOWN},
        {5,3,DIR_LEFT},  {5,5,DIR_UP}
    }},
    // 6 — 6x6, 14 arrows
    { 6, 6, 14, {
        {0,0,DIR_UP},    {0,2,DIR_RIGHT}, {0,5,DIR_RIGHT},
        {1,1,DIR_LEFT},  {1,4,DIR_DOWN},
        {2,0,DIR_DOWN},  {2,3,DIR_RIGHT}, {2,5,DIR_UP},
        {3,1,DIR_UP},    {3,4,DIR_LEFT},
        {4,0,DIR_RIGHT}, {4,2,DIR_DOWN},  {4,5,DIR_DOWN},
        {5,3,DIR_LEFT}
    }},
    // 7 — 7x7, 16 arrows
    { 7, 7, 16, {
        {0,0,DIR_LEFT},  {0,3,DIR_UP},    {0,6,DIR_RIGHT},
        {1,1,DIR_UP},    {1,5,DIR_DOWN},
        {2,2,DIR_RIGHT}, {2,4,DIR_LEFT},  {2,6,DIR_DOWN},
        {3,0,DIR_DOWN},  {3,6,DIR_UP},
        {4,1,DIR_RIGHT}, {4,4,DIR_UP},    {4,5,DIR_LEFT},
        {5,2,DIR_DOWN},  {5,6,DIR_LEFT},
        {6,0,DIR_RIGHT}
    }},
    // 8 — 7x7, 18 arrows
    { 7, 7, 18, {
        {0,1,DIR_UP},    {0,3,DIR_RIGHT}, {0,6,DIR_LEFT},
        {1,0,DIR_DOWN},  {1,4,DIR_UP},    {1,6,DIR_DOWN},
        {2,2,DIR_LEFT},  {2,5,DIR_RIGHT},
        {3,0,DIR_RIGHT}, {3,3,DIR_DOWN},  {3,6,DIR_LEFT},
        {4,1,DIR_DOWN},  {4,4,DIR_LEFT},  {4,6,DIR_UP},
        {5,2,DIR_RIGHT}, {5,5,DIR_UP},
        {6,0,DIR_UP},    {6,3,DIR_LEFT}
    }}
};

//=============================================================================
// Colors
//=============================================================================
#define CLR32(r,g,b,a) C2D_Color32((r),(g),(b),(a))

static const u32 CLR_BG          = CLR32(0x1A,0x1A,0x2E,0xFF);
static const u32 CLR_PANEL       = CLR32(0x0D,0x14,0x22,0xFF);
static const u32 CLR_CELL        = CLR32(0x0F,0x34,0x60,0xFF);
static const u32 CLR_CELL_SEL    = CLR32(0x1E,0x55,0x90,0xFF);
static const u32 CLR_CELL_FLASH  = CLR32(0x55,0x08,0x08,0xFF);
static const u32 CLR_WHITE       = CLR32(0xFF,0xFF,0xFF,0xFF);
static const u32 CLR_YELLOW      = CLR32(0xF5,0xC8,0x42,0xFF);
static const u32 CLR_RED         = CLR32(0xFF,0x44,0x44,0xFF);
static const u32 CLR_GRAY        = CLR32(0x88,0x88,0x88,0xFF);
static const u32 CLR_CURSOR_BORDER = CLR32(0xFF,0xFF,0xFF,0xAA);

// Arrow body colors per direction [R,L,U,D]
static const u32 CLR_ARR[4] = {
    CLR32(0xE7,0x4C,0x3C,0xFF),   // RIGHT - red
    CLR32(0x34,0x98,0xDB,0xFF),   // LEFT  - blue
    CLR32(0x2E,0xCC,0x71,0xFF),   // UP    - green
    CLR32(0x9B,0x59,0xB6,0xFF),   // DOWN  - purple
};
static const u32 CLR_ARR_HINT[4] = {
    CLR32(0xFF,0xA0,0x40,0xFF),
    CLR32(0x80,0xD8,0xFF,0xFF),
    CLR32(0x90,0xFF,0xB0,0xFF),
    CLR32(0xD8,0xA0,0xFF,0xFF),
};

//=============================================================================
// Game state
//=============================================================================
static struct {
    // Level
    int         levelIdx;
    int         hearts;
    int         rows, cols;
    Cell        grid[MAX_ROWS][MAX_COLS];
    int         arrowsLeft;

    // Camera
    float       camX, camY;
    float       zoom;

    // D-pad cursor
    int         curR, curC;

    // Hint
    bool        hintActive;
    int         hintR, hintC;

    // Fly animation
    bool        flyActive;
    int         flyR, flyC;
    Direction   flyDir;
    float       flyProgress;   // 0..1+ (used to offset drawing)

    // Flash (wrong move)
    bool        flashActive;
    int         flashR, flashC;
    int         flashTimer;

    // Overall state
    GameStatus  status;

} G;

// Text rendering helper (one static buf, cleared per call)
static C2D_TextBuf s_textBuf = nullptr;

static void drawText(const char* str, float x, float y, float z,
                     float scale, u32 color)
{
    if (!s_textBuf) s_textBuf = C2D_TextBufNew(128);
    C2D_TextBufClear(s_textBuf);
    C2D_Text t;
    C2D_TextParse(&t, s_textBuf, str);
    C2D_TextOptimize(&t);
    C2D_DrawText(&t, C2D_WithColor, x, y, z, scale, scale, color);
}

//=============================================================================
// Grid helpers
//=============================================================================
static int countArrows()
{
    int n = 0;
    for (int r = 0; r < G.rows; r++)
        for (int c = 0; c < G.cols; c++)
            if (G.grid[r][c].active) n++;
    return n;
}

static bool canEscape(int r, int c)
{
    Direction d = G.grid[r][c].dir;
    switch (d) {
        case DIR_RIGHT: for (int cc=c+1; cc<G.cols; cc++) if (G.grid[r][cc].active) return false; break;
        case DIR_LEFT:  for (int cc=c-1; cc>=0;     cc--) if (G.grid[r][cc].active) return false; break;
        case DIR_UP:    for (int rr=r-1; rr>=0;     rr--) if (G.grid[rr][c].active) return false; break;
        case DIR_DOWN:  for (int rr=r+1; rr<G.rows; rr++) if (G.grid[rr][c].active) return false; break;
    }
    return true;
}

static void findHint()
{
    for (int r = 0; r < G.rows; r++)
        for (int c = 0; c < G.cols; c++)
            if (G.grid[r][c].active && canEscape(r, c)) {
                G.hintR = r; G.hintC = c;
                G.hintActive = true;
                return;
            }
}

//=============================================================================
// Level loading
//=============================================================================
static void loadLevel(int idx)
{
    idx = idx % NUM_LEVELS;
    const LevelDef& lvl = LEVELS[idx];

    G.levelIdx  = idx;
    G.rows      = lvl.rows;
    G.cols      = lvl.cols;

    memset(G.grid, 0, sizeof(G.grid));
    for (int i = 0; i < lvl.count; i++) {
        G.grid[lvl.arrows[i].r][lvl.arrows[i].c].active = true;
        G.grid[lvl.arrows[i].r][lvl.arrows[i].c].dir    = lvl.arrows[i].d;
    }

    G.arrowsLeft = lvl.count;
    G.camX = 0; G.camY = 0;
    G.zoom = 1.0f;

    // Place cursor on first active cell
    G.curR = 0; G.curC = 0;
    for (int r = 0; r < G.rows && !G.grid[G.curR][G.curC].active; r++)
        for (int c = 0; c < G.cols; c++)
            if (G.grid[r][c].active) { G.curR=r; G.curC=c; goto cursor_found; }
    cursor_found:;

    G.hintActive  = false;
    G.flyActive   = false;
    G.flashActive = false;
    G.status      = GS_PLAYING;
}

//=============================================================================
// Cell tap logic
//=============================================================================
static void tapCell(int r, int c)
{
    if (r < 0 || r >= G.rows || c < 0 || c >= G.cols) return;
    if (!G.grid[r][c].active) return;
    if (G.flyActive) return;
    G.hintActive = false;

    if (canEscape(r, c)) {
        // Remove from grid immediately (logic) — animate the fly visually
        G.flyActive   = true;
        G.flyR        = r;
        G.flyC        = c;
        G.flyDir      = G.grid[r][c].dir;
        G.flyProgress = 0.0f;
        G.grid[r][c].active = false;
        G.arrowsLeft--;
        G.status = GS_FLYING;
    } else {
        // Wrong move
        G.hearts--;
        G.flashActive = true;
        G.flashR = r; G.flashC = c;
        G.flashTimer = FLASH_FRAMES;
        if (G.hearts <= 0) G.status = GS_OVER;
    }
}

//=============================================================================
// Camera helpers
//=============================================================================
static void getGridOrigin(int screenW, int screenH, float* ox, float* oy)
{
    float cellSize = CELL_BASE * G.zoom;
    float gridW    = G.cols * cellSize;
    float gridH    = G.rows * cellSize;
    *ox = (screenW - gridW) / 2.0f + G.camX;
    *oy = (screenH - gridH) / 2.0f + G.camY;
}

// Map screen touch coordinate to grid cell
static void touchToCell(int tx, int ty, int screenW, int screenH,
                        int* outR, int* outC)
{
    float cellSize = CELL_BASE * G.zoom;
    float ox, oy;
    getGridOrigin(screenW, screenH, &ox, &oy);
    *outC = (int)((tx - ox) / cellSize);
    *outR = (int)((ty - oy) / cellSize);
}

//=============================================================================
// Draw a single arrow (circle + directional triangle)
//=============================================================================
static void drawArrow(float cx, float cy, float cellSize,
                      Direction dir, bool hinted, bool flashing,
                      float flyOffset)
{
    // Apply fly offset (direction-based displacement)
    if (flyOffset > 0.0f) {
        float dist = flyOffset * (600.0f + cellSize);
        switch (dir) {
            case DIR_RIGHT: cx += dist; break;
            case DIR_LEFT:  cx -= dist; break;
            case DIR_UP:    cy -= dist; break;
            case DIR_DOWN:  cy += dist; break;
        }
    }

    float r = cellSize * 0.36f;     // circle radius
    float a = cellSize * 0.20f;     // arrowhead arm
    float b = a * 1.6f;             // arrowhead tip reach

    u32 bodyColor = flashing ? CLR32(0xFF,0x22,0x22,0xFF)
                  : hinted   ? CLR_ARR_HINT[dir]
                  : CLR_ARR[dir];

    // Body circle
    C2D_DrawCircleSolid(cx, cy, 0, r, bodyColor);

    // Arrowhead triangle (white)
    switch (dir) {
        case DIR_RIGHT:
            C2D_DrawTriangle(cx-a, cy-a, CLR_WHITE,
                             cx-a, cy+a, CLR_WHITE,
                             cx+b, cy,   CLR_WHITE, 0);
            break;
        case DIR_LEFT:
            C2D_DrawTriangle(cx+a, cy-a, CLR_WHITE,
                             cx+a, cy+a, CLR_WHITE,
                             cx-b, cy,   CLR_WHITE, 0);
            break;
        case DIR_UP:
            C2D_DrawTriangle(cx-a, cy+a, CLR_WHITE,
                             cx+a, cy+a, CLR_WHITE,
                             cx,   cy-b, CLR_WHITE, 0);
            break;
        case DIR_DOWN:
            C2D_DrawTriangle(cx-a, cy-a, CLR_WHITE,
                             cx+a, cy-a, CLR_WHITE,
                             cx,   cy+b, CLR_WHITE, 0);
            break;
    }
}

//=============================================================================
// Draw grid + flying arrow on a given screen
//=============================================================================
static void drawGrid(int screenW, int screenH)
{
    float cellSize = CELL_BASE * G.zoom;
    float gap      = (cellSize > 12.0f) ? 4.0f : 2.0f;
    float inner    = cellSize - gap * 2.0f;
    float ox, oy;
    getGridOrigin(screenW, screenH, &ox, &oy);

    // Panel background
    float pw = G.cols * cellSize + gap * 2.0f;
    float ph = G.rows * cellSize + gap * 2.0f;
    C2D_DrawRectSolid(ox - gap, oy - gap, 0, pw, ph, CLR_PANEL);

    for (int r = 0; r < G.rows; r++) {
        for (int c = 0; c < G.cols; c++) {
            float x  = ox + c * cellSize + gap;
            float y  = oy + r * cellSize + gap;
            float cx = x + inner * 0.5f;
            float cy = y + inner * 0.5f;

            bool isCursor = (r == G.curR  && c == G.curC);
            bool isFlash  = (G.flashActive && G.flashR == r && G.flashC == c);

            // Cell background
            u32 cellClr = isFlash ? CLR_CELL_FLASH
                        : isCursor ? CLR_CELL_SEL
                        : CLR_CELL;
            C2D_DrawRectSolid(x, y, 0, inner, inner, cellClr);

            // Arrow (if present in grid AND not currently flying)
            if (G.grid[r][c].active) {
                bool hinted = G.hintActive && G.hintR == r && G.hintC == c;
                drawArrow(cx, cy, cellSize, G.grid[r][c].dir,
                          hinted, isFlash, 0.0f);
            }

            // Cursor border
            if (isCursor) {
                float bw = 2.0f;
                C2D_DrawRectSolid(x,            y,             0, inner, bw,    CLR_CURSOR_BORDER);
                C2D_DrawRectSolid(x,            y+inner-bw,    0, inner, bw,    CLR_CURSOR_BORDER);
                C2D_DrawRectSolid(x,            y,             0, bw,    inner, CLR_CURSOR_BORDER);
                C2D_DrawRectSolid(x+inner-bw,   y,             0, bw,    inner, CLR_CURSOR_BORDER);
            }
        }
    }

    // Flying arrow overlay
    if (G.flyActive) {
        float cx = ox + (G.flyC + 0.5f) * cellSize;
        float cy = oy + (G.flyR + 0.5f) * cellSize;
        drawArrow(cx, cy, cellSize, G.flyDir, false, false, G.flyProgress);
    }
}

//=============================================================================
// Draw HUD overlay
//=============================================================================
static void drawHUD(int screenW, int screenH)
{
    char buf[64];

    // Level
    sprintf(buf, "LVL %d", G.levelIdx + 1);
    drawText(buf, 5, 4, 0, 0.55f, CLR_YELLOW);

    // Remaining arrows
    sprintf(buf, "%d left", G.arrowsLeft);
    float tw = (float)(strlen(buf)) * 7.0f * 0.55f;
    drawText(buf, screenW / 2.0f - tw / 2.0f, 4, 0, 0.55f, CLR_WHITE);

    // Hearts
    const char* hstr[] = { "HP:---", "HP:*--", "HP:**-", "HP:***" };
    drawText(hstr[G.hearts < 0 ? 0 : G.hearts > 3 ? 3 : G.hearts],
             screenW - 58, 4, 0, 0.55f,
             G.hearts <= 1 ? CLR_RED : CLR32(0xFF,0x80,0x80,0xFF));

    // Bottom hint bar (only on narrower screen)
    if (screenW <= BOT_W) {
        drawText("Y=Hint  SEL=Restart  L/R=Zoom", 4, screenH - 13, 0,
                 0.40f, CLR_GRAY);
    }

    // Overlay messages
    if (G.status == GS_CLEAR || G.status == GS_OVER || G.status == GS_PAUSED) {
        float bw = 160, bh = 52;
        float bx = screenW / 2.0f - bw / 2.0f;
        float by = screenH / 2.0f - bh / 2.0f;
        C2D_DrawRectSolid(bx, by, 0, bw, bh, CLR32(0x10,0x18,0x30,0xE0));

        if (G.status == GS_CLEAR) {
            drawText("LEVEL CLEAR!",    bx+18, by+6,  0, 0.70f, CLR_YELLOW);
            drawText("A / Touch: Next", bx+12, by+30, 0, 0.48f, CLR_WHITE);
        } else if (G.status == GS_OVER) {
            drawText("GAME OVER",       bx+28, by+6,  0, 0.70f, CLR_RED);
            drawText("A / Touch: Retry",bx+12, by+30, 0, 0.48f, CLR_WHITE);
        } else {
            drawText("PAUSED",          bx+40, by+6,  0, 0.70f, CLR_WHITE);
            drawText("START to resume", bx+12, by+30, 0, 0.48f, CLR_GRAY);
        }
    }
}

//=============================================================================
// Update (called once per frame)
//=============================================================================
static void update()
{
    // Fly animation tick
    if (G.flyActive) {
        G.flyProgress += FLY_SPEED;
        if (G.flyProgress >= 1.0f) {
            G.flyActive = false;
            G.status    = (G.arrowsLeft == 0) ? GS_CLEAR : GS_PLAYING;
        }
    }

    // Flash timer
    if (G.flashActive) {
        G.flashTimer--;
        if (G.flashTimer <= 0) G.flashActive = false;
    }
}

//=============================================================================
// Input (called once per frame)
//=============================================================================
static void handleInput()
{
    hidScanInput();
    u32 kDown = hidKeysDown();

    // Pause toggle (always available except on overlay screens)
    if (kDown & KEY_START) {
        if (G.status == GS_PLAYING || G.status == GS_FLYING) {
            G.status = GS_PAUSED;
            return;
        } else if (G.status == GS_PAUSED) {
            G.status = G.flyActive ? GS_FLYING : GS_PLAYING;
            return;
        }
    }

    if (G.status == GS_PAUSED) return;

    // Clear / over: advance/retry on A or touch
    if (G.status == GS_CLEAR) {
        bool advance = (kDown & KEY_A) || (kDown & KEY_TOUCH);
        if (advance) {
            G.hearts = MAX_HEARTS;
            loadLevel(G.levelIdx + 1);
        }
        return;
    }
    if (G.status == GS_OVER) {
        bool retry = (kDown & KEY_A) || (kDown & KEY_TOUCH);
        if (retry) {
            G.hearts = MAX_HEARTS;
            loadLevel(G.levelIdx);
        }
        return;
    }

    // SELECT = restart level
    if (kDown & KEY_SELECT) {
        G.hearts = MAX_HEARTS;
        loadLevel(G.levelIdx);
        return;
    }

    // Block most input during fly animation
    if (G.flyActive) return;

    // L / R = zoom
    if (kDown & KEY_L) { G.zoom = fmaxf(G.zoom - ZOOM_STEP, ZOOM_MIN); }
    if (kDown & KEY_R) { G.zoom = fminf(G.zoom + ZOOM_STEP, ZOOM_MAX); }

    // Circle pad = pan
    circlePosition cpad;
    hidCircleRead(&cpad);
    if (abs(cpad.dx) > CPAD_DEADZONE) G.camX += (cpad.dx / 156.0f) * CAM_SPEED;
    if (abs(cpad.dy) > CPAD_DEADZONE) G.camY -= (cpad.dy / 156.0f) * CAM_SPEED;

    // D-Pad = move cursor
    if (kDown & KEY_DRIGHT) G.curC = (G.curC + 1) % G.cols;
    if (kDown & KEY_DLEFT)  G.curC = (G.curC - 1 + G.cols) % G.cols;
    if (kDown & KEY_DUP)    G.curR = (G.curR - 1 + G.rows) % G.rows;
    if (kDown & KEY_DDOWN)  G.curR = (G.curR + 1) % G.rows;

    // A = tap cursor
    if (kDown & KEY_A) {
        tapCell(G.curR, G.curC);
        return;
    }

    // Y = hint toggle
    if (kDown & KEY_Y) {
        if (G.hintActive) G.hintActive = false;
        else findHint();
        return;
    }

    // Touch (bottom screen)
    if (kDown & KEY_TOUCH) {
        touchPosition touch;
        hidTouchRead(&touch);
        int tr, tc;
        touchToCell(touch.px, touch.py, BOT_W, BOT_H, &tr, &tc);
        tapCell(tr, tc);
    }
}

//=============================================================================
// Main
//=============================================================================
int main(int argc, char* argv[])
{
    // Init hardware
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Create render targets
    C3D_RenderTarget* topScreen = C2D_CreateScreenTarget(GFX_TOP,    GFX_LEFT);
    C3D_RenderTarget* botScreen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // Init game
    G.hearts = MAX_HEARTS;
    loadLevel(0);

    // Main loop
    while (aptMainLoop())
    {
        handleInput();
        update();

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // ── Top screen ──────────────────────────────────────────────
        C2D_TargetClear(topScreen, CLR_BG);
        C2D_SceneBegin(topScreen);
        drawGrid(TOP_W, TOP_H);
        drawHUD(TOP_W, TOP_H);

        // ── Bottom screen (interactive) ─────────────────────────────
        C2D_TargetClear(botScreen, CLR_BG);
        C2D_SceneBegin(botScreen);
        drawGrid(BOT_W, BOT_H);
        drawHUD(BOT_W, BOT_H);

        C3D_FrameEnd(0);
    }

    // Cleanup
    if (s_textBuf) C2D_TextBufDelete(s_textBuf);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
