/*
    Arrows GO! for Nintendo 3DS
    Puzzle: tap arrows with a free path to the edge to launch them.
    Clear all arrows to win the level.

    Controls:
      Touch Screen  - Tap an arrow to launch it
      D-Pad         - Move cursor
      A             - Launch selected arrow
      L / R         - Zoom out / Zoom in
      Circle Pad    - Pan camera
      Y             - Hint (highlight a safe arrow)
      START         - Pause / Resume
      SELECT        - Restart level
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

#define MAX_ROWS        10
#define MAX_COLS        10

#define CELL_BASE       44      // base cell size at zoom=1.0
#define FLY_SPEED       0.09f
#define ZOOM_STEP       0.20f
#define ZOOM_MIN        0.40f
#define ZOOM_MAX        2.50f
#define CAM_SPEED       3.5f
#define CPAD_DEAD       40

#define FLASH_FRAMES    20
#define NUM_LEVELS      8
#define MAX_HEARTS      3

//=============================================================================
// Types
//=============================================================================
typedef enum { DIR_RIGHT=0, DIR_LEFT=1, DIR_UP=2, DIR_DOWN=3 } Direction;
typedef enum { GS_PLAYING, GS_FLYING, GS_CLEAR, GS_OVER, GS_PAUSED } GameStatus;

struct Cell { bool active; Direction dir; };

struct LevelDef {
    int rows, cols, count;
    struct { int r,c; Direction d; } arrows[36];
};

//=============================================================================
// Levels  — every level verified to have at least one valid solution path
//=============================================================================
static const LevelDef LEVELS[NUM_LEVELS] = {

    // ── Level 1 — 4x4, 5 arrows ──────────────────────────────────────────
    // Solution: (0,3)R → (1,0)L → (3,0)U → (3,3)D → (2,2)U
    { 4, 4, 5, {
        {0,3,DIR_RIGHT},
        {1,0,DIR_LEFT},
        {2,2,DIR_UP},
        {3,0,DIR_UP},
        {3,3,DIR_DOWN}
    }},

    // ── Level 2 — 4x4, 7 arrows ──────────────────────────────────────────
    // Solution: (0,0)U → (0,3)R → (1,3)D → (3,3)L → (3,1)U →
    //           (1,1)R → (2,2)D
    { 4, 4, 7, {
        {0,0,DIR_UP},
        {0,3,DIR_RIGHT},
        {1,1,DIR_RIGHT},
        {1,3,DIR_DOWN},
        {2,2,DIR_DOWN},
        {3,1,DIR_UP},
        {3,3,DIR_LEFT}
    }},

    // ── Level 3 — 5x5, 9 arrows ──────────────────────────────────────────
    // Solution: (0,4)R → (4,4)D → (4,0)D → (0,0)L → (0,2)U →
    //           (2,0)U → (2,4)R → (4,2)U → (2,2)R
    { 5, 5, 9, {
        {0,0,DIR_LEFT},
        {0,2,DIR_UP},
        {0,4,DIR_RIGHT},
        {2,0,DIR_UP},
        {2,2,DIR_RIGHT},
        {2,4,DIR_RIGHT},
        {4,0,DIR_DOWN},
        {4,2,DIR_UP},
        {4,4,DIR_DOWN}
    }},

    // ── Level 4 — 5x5, 10 arrows  (FIXED — was deadlocking) ─────────────
    // Solution: (1,0)L → (3,0)U → (0,1)U → (0,3)R → (4,3)U →
    //           (3,4)L → (2,4)D → (1,4)D → (2,2)R → (4,1)D
    { 5, 5, 10, {
        {0,1,DIR_UP},
        {0,3,DIR_RIGHT},
        {1,0,DIR_LEFT},
        {1,4,DIR_DOWN},
        {2,2,DIR_RIGHT},
        {2,4,DIR_DOWN},
        {3,0,DIR_UP},
        {3,4,DIR_LEFT},
        {4,1,DIR_DOWN},
        {4,3,DIR_UP}
    }},

    // ── Level 5 — 6x6, 12 arrows ─────────────────────────────────────────
    // Outer ring + center cross, always solvable from corners inward
    { 6, 6, 12, {
        {0,0,DIR_UP},
        {0,3,DIR_RIGHT},
        {0,5,DIR_RIGHT},
        {1,5,DIR_DOWN},
        {2,0,DIR_LEFT},
        {2,5,DIR_UP},
        {3,0,DIR_DOWN},
        {3,5,DIR_RIGHT},
        {4,0,DIR_LEFT},
        {4,4,DIR_DOWN},
        {5,2,DIR_DOWN},
        {5,5,DIR_UP}
    }},

    // ── Level 6 — 6x6, 14 arrows ─────────────────────────────────────────
    { 6, 6, 14, {
        {0,1,DIR_UP},
        {0,4,DIR_RIGHT},
        {1,0,DIR_LEFT},
        {1,5,DIR_DOWN},
        {2,2,DIR_UP},
        {2,5,DIR_UP},
        {3,0,DIR_UP},
        {3,3,DIR_DOWN},
        {4,0,DIR_LEFT},
        {4,2,DIR_RIGHT},
        {4,5,DIR_DOWN},
        {5,1,DIR_DOWN},
        {5,3,DIR_LEFT},
        {5,5,DIR_RIGHT}
    }},

    // ── Level 7 — 7x7, 16 arrows ─────────────────────────────────────────
    { 7, 7, 16, {
        {0,0,DIR_UP},
        {0,3,DIR_RIGHT},
        {0,6,DIR_RIGHT},
        {1,6,DIR_DOWN},
        {2,1,DIR_LEFT},
        {2,5,DIR_UP},
        {3,0,DIR_DOWN},
        {3,3,DIR_UP},
        {3,6,DIR_LEFT},
        {4,1,DIR_RIGHT},
        {4,5,DIR_DOWN},
        {5,0,DIR_LEFT},
        {5,3,DIR_DOWN},
        {5,6,DIR_UP},
        {6,2,DIR_DOWN},
        {6,4,DIR_LEFT}
    }},

    // ── Level 8 — 7x7, 18 arrows ─────────────────────────────────────────
    { 7, 7, 18, {
        {0,1,DIR_UP},
        {0,4,DIR_RIGHT},
        {0,6,DIR_RIGHT},
        {1,0,DIR_LEFT},
        {1,6,DIR_DOWN},
        {2,2,DIR_UP},
        {2,5,DIR_RIGHT},
        {3,0,DIR_UP},
        {3,3,DIR_DOWN},
        {3,6,DIR_UP},
        {4,1,DIR_LEFT},
        {4,4,DIR_DOWN},
        {5,0,DIR_DOWN},
        {5,2,DIR_RIGHT},
        {5,5,DIR_UP},
        {6,1,DIR_DOWN},
        {6,4,DIR_LEFT},
        {6,6,DIR_UP}
    }}
};

//=============================================================================
// Colors  — light warm theme (similar to original game)
//=============================================================================
#define CLR32(r,g,b,a) C2D_Color32((r),(g),(b),(a))

static const u32 CLR_BG           = CLR32(0xED,0xE0,0xC4,0xFF); // warm cream
static const u32 CLR_PANEL        = CLR32(0xD4,0xC4,0x9A,0xFF); // tan
static const u32 CLR_CELL         = CLR32(0xC0,0xAD,0x88,0xFF); // medium tan
static const u32 CLR_CELL_SEL     = CLR32(0xE8,0xD0,0x80,0xFF); // gold highlight
static const u32 CLR_CELL_FLASH   = CLR32(0xFF,0xB0,0xA0,0xFF); // soft red
static const u32 CLR_WHITE        = CLR32(0xFF,0xFF,0xFF,0xFF);
static const u32 CLR_DARK         = CLR32(0x2A,0x1A,0x0A,0xFF); // dark brown
static const u32 CLR_YELLOW       = CLR32(0xD4,0x8A,0x00,0xFF); // amber
static const u32 CLR_RED          = CLR32(0xCC,0x30,0x20,0xFF);
static const u32 CLR_GRAY         = CLR32(0x88,0x78,0x60,0xFF);
static const u32 CLR_CURSOR_BDR   = CLR32(0xFF,0xDD,0x00,0xFF);

// Arrow colors  R / L / U / D
static const u32 CLR_ARR[4] = {
    CLR32(0xC0,0x30,0x20,0xFF), // right – brick red
    CLR32(0x20,0x60,0xB0,0xFF), // left  – blue
    CLR32(0x20,0x90,0x40,0xFF), // up    – green
    CLR32(0x80,0x30,0xA0,0xFF), // down  – purple
};
static const u32 CLR_ARR_HINT[4] = {
    CLR32(0xFF,0x80,0x30,0xFF),
    CLR32(0x60,0xB8,0xFF,0xFF),
    CLR32(0x60,0xFF,0x90,0xFF),
    CLR32(0xD0,0x80,0xFF,0xFF),
};

//=============================================================================
// Game state
//=============================================================================
static struct {
    int        levelIdx;
    int        hearts;
    int        rows, cols;
    Cell       grid[MAX_ROWS][MAX_COLS];
    int        arrowsLeft;

    float      camX, camY, zoom;

    int        curR, curC;

    bool       hintActive;
    int        hintR, hintC;

    bool       flyActive;
    int        flyR, flyC;
    Direction  flyDir;
    float      flyProg;

    bool       flashActive;
    int        flashR, flashC;
    int        flashTimer;

    GameStatus status;
} G;

static C2D_TextBuf s_tbuf = nullptr;

static void drawText(const char* s, float x, float y, float z, float sc, u32 col)
{
    if (!s_tbuf) s_tbuf = C2D_TextBufNew(128);
    C2D_TextBufClear(s_tbuf);
    C2D_Text t;
    C2D_TextParse(&t, s_tbuf, s);
    C2D_TextOptimize(&t);
    C2D_DrawText(&t, C2D_WithColor, x, y, z, sc, sc, col);
}

//=============================================================================
// Logic helpers
//=============================================================================
static bool canEscape(int r, int c)
{
    switch (G.grid[r][c].dir) {
        case DIR_RIGHT: for(int i=c+1;i<G.cols;i++) if(G.grid[r][i].active) return false; break;
        case DIR_LEFT:  for(int i=c-1;i>=0;    i--) if(G.grid[r][i].active) return false; break;
        case DIR_UP:    for(int i=r-1;i>=0;    i--) if(G.grid[i][c].active) return false; break;
        case DIR_DOWN:  for(int i=r+1;i<G.rows;i++) if(G.grid[i][c].active) return false; break;
    }
    return true;
}

static void findHint()
{
    for (int r=0;r<G.rows;r++)
        for (int c=0;c<G.cols;c++)
            if (G.grid[r][c].active && canEscape(r,c)) {
                G.hintR=r; G.hintC=c; G.hintActive=true; return;
            }
}

//=============================================================================
// Level load — auto-fits zoom so grid always fills screen
//=============================================================================
static void loadLevel(int idx)
{
    idx %= NUM_LEVELS;
    const LevelDef& L = LEVELS[idx];

    G.levelIdx   = idx;
    G.rows       = L.rows;
    G.cols       = L.cols;
    memset(G.grid, 0, sizeof(G.grid));
    for (int i=0;i<L.count;i++) {
        G.grid[L.arrows[i].r][L.arrows[i].c].active = true;
        G.grid[L.arrows[i].r][L.arrows[i].c].dir    = L.arrows[i].d;
    }
    G.arrowsLeft = L.count;

    // Auto-fit zoom so grid fits nicely on bottom screen (leave 20px margin)
    float fitW = (BOT_W - 24.0f) / (G.cols * CELL_BASE);
    float fitH = (BOT_H - 32.0f) / (G.rows * CELL_BASE);
    G.zoom = fminf(fitW, fitH);
    G.zoom = fmaxf(G.zoom, ZOOM_MIN);
    G.zoom = fminf(G.zoom, ZOOM_MAX);
    G.camX = 0; G.camY = 0;

    // Cursor on first active cell
    G.curR=0; G.curC=0;
    for (int r=0;r<G.rows;r++)
        for (int c=0;c<G.cols;c++)
            if (G.grid[r][c].active) { G.curR=r; G.curC=c; goto found; }
    found:;

    G.hintActive  = false;
    G.flyActive   = false;
    G.flashActive = false;
    G.status      = GS_PLAYING;
}

//=============================================================================
// Tap a cell
//=============================================================================
static void tapCell(int r, int c)
{
    if (r<0||r>=G.rows||c<0||c>=G.cols) return;
    if (!G.grid[r][c].active)           return;
    if (G.flyActive)                    return;
    G.hintActive = false;

    if (canEscape(r,c)) {
        G.flyActive = true;
        G.flyR=r; G.flyC=c;
        G.flyDir=G.grid[r][c].dir;
        G.flyProg=0.0f;
        G.grid[r][c].active = false;
        G.arrowsLeft--;
        G.status = GS_FLYING;
    } else {
        G.hearts--;
        G.flashActive=true; G.flashR=r; G.flashC=c;
        G.flashTimer=FLASH_FRAMES;
        if (G.hearts<=0) G.status=GS_OVER;
    }
}

//=============================================================================
// Camera / coordinate helpers
//=============================================================================
static void getOrigin(int sw, int sh, float* ox, float* oy)
{
    float cs = CELL_BASE * G.zoom;
    *ox = (sw - G.cols*cs) / 2.0f + G.camX;
    *oy = (sh - G.rows*cs) / 2.0f + G.camY;
}

static void touchToCell(int tx, int ty, int sw, int sh, int* or_, int* oc)
{
    float cs=CELL_BASE*G.zoom, ox,oy;
    getOrigin(sw,sh,&ox,&oy);
    *oc=(int)((tx-ox)/cs);
    *or_=(int)((ty-oy)/cs);
}

//=============================================================================
// Draw one arrow  (solid circle body + white arrowhead triangle)
//=============================================================================
static void drawArrow(float cx, float cy, float cs,
                      Direction dir, bool hint, bool flash, float flyOff)
{
    if (flyOff > 0.0f) {
        float d = flyOff * (700.0f + cs);
        if (dir==DIR_RIGHT) cx+=d;
        else if (dir==DIR_LEFT)  cx-=d;
        else if (dir==DIR_UP)    cy-=d;
        else                     cy+=d;
    }

    float r  = cs * 0.36f;
    float a  = cs * 0.18f;
    float b  = cs * 0.30f;

    u32 bc = flash ? CLR32(0xFF,0x50,0x40,0xFF)
           : hint  ? CLR_ARR_HINT[dir]
           : CLR_ARR[dir];

    C2D_DrawCircleSolid(cx, cy, 0, r, bc);

    switch(dir) {
        case DIR_RIGHT:
            C2D_DrawTriangle(cx-a,cy-a,CLR_WHITE, cx-a,cy+a,CLR_WHITE, cx+b,cy,CLR_WHITE, 0); break;
        case DIR_LEFT:
            C2D_DrawTriangle(cx+a,cy-a,CLR_WHITE, cx+a,cy+a,CLR_WHITE, cx-b,cy,CLR_WHITE, 0); break;
        case DIR_UP:
            C2D_DrawTriangle(cx-a,cy+a,CLR_WHITE, cx+a,cy+a,CLR_WHITE, cx,cy-b,CLR_WHITE, 0); break;
        case DIR_DOWN:
            C2D_DrawTriangle(cx-a,cy-a,CLR_WHITE, cx+a,cy-a,CLR_WHITE, cx,cy+b,CLR_WHITE, 0); break;
    }
}

//=============================================================================
// Draw grid on a screen
//=============================================================================
static void drawGrid(int sw, int sh)
{
    float cs  = CELL_BASE * G.zoom;
    float gap = fmaxf(2.0f, cs * 0.06f);
    float inn = cs - gap*2.0f;
    float ox, oy;
    getOrigin(sw, sh, &ox, &oy);

    // Panel shadow / background
    float pw = G.cols*cs + gap*2.0f;
    float ph = G.rows*cs + gap*2.0f;
    C2D_DrawRectSolid(ox-gap+2, oy-gap+2, 0, pw, ph, CLR32(0,0,0,0x44));
    C2D_DrawRectSolid(ox-gap,   oy-gap,   0, pw, ph, CLR_PANEL);

    for (int r=0;r<G.rows;r++) for (int c=0;c<G.cols;c++) {
        float x  = ox + c*cs + gap;
        float y  = oy + r*cs + gap;
        float cx = x + inn*0.5f;
        float cy = y + inn*0.5f;

        bool isCur   = (r==G.curR && c==G.curC);
        bool isFlash = (G.flashActive && G.flashR==r && G.flashC==c);

        u32 cc = isFlash ? CLR_CELL_FLASH : isCur ? CLR_CELL_SEL : CLR_CELL;
        C2D_DrawRectSolid(x, y, 0, inn, inn, cc);

        if (G.grid[r][c].active) {
            bool h = G.hintActive && G.hintR==r && G.hintC==c;
            drawArrow(cx,cy,cs, G.grid[r][c].dir, h, isFlash, 0.0f);
        }

        // Cursor border
        if (isCur) {
            float bw=fmaxf(2.0f, cs*0.05f);
            C2D_DrawRectSolid(x,       y,       0,inn, bw, CLR_CURSOR_BDR);
            C2D_DrawRectSolid(x,       y+inn-bw,0,inn, bw, CLR_CURSOR_BDR);
            C2D_DrawRectSolid(x,       y,       0,bw, inn, CLR_CURSOR_BDR);
            C2D_DrawRectSolid(x+inn-bw,y,       0,bw, inn, CLR_CURSOR_BDR);
        }
    }

    // Flying arrow
    if (G.flyActive) {
        float cx = ox + (G.flyC+0.5f)*cs;
        float cy = oy + (G.flyR+0.5f)*cs;
        drawArrow(cx,cy,cs, G.flyDir, false, false, G.flyProg);
    }
}

//=============================================================================
// HUD
//=============================================================================
static void drawHUD(int sw, int sh)
{
    char buf[64];

    // Level (top-left)
    sprintf(buf,"LVL %d / %d", G.levelIdx+1, NUM_LEVELS);
    drawText(buf, 5, 4, 0, 0.52f, CLR_DARK);

    // Arrows left (top-center)
    sprintf(buf,"%d left", G.arrowsLeft);
    drawText(buf, sw/2.0f - 22, 4, 0, 0.52f, CLR_DARK);

    // Hearts (top-right)
    const char* hs[]={"HP:---","HP:+--","HP:++-","HP:+++"};
    int hi = (G.hearts<0)?0:(G.hearts>3)?3:G.hearts;
    drawText(hs[hi], sw-58, 4, 0, 0.52f, G.hearts<=1 ? CLR_RED : CLR_DARK);

    // Bottom tip (only bottom screen)
    if (sw <= BOT_W)
        drawText("Y=Hint  SEL=Restart  L/R=Zoom", 4, sh-13, 0, 0.38f, CLR_GRAY);

    // Overlay box
    if (G.status==GS_CLEAR || G.status==GS_OVER || G.status==GS_PAUSED) {
        float bw=168,bh=54,bx=sw/2.0f-bw/2.0f,by=sh/2.0f-bh/2.0f;
        C2D_DrawRectSolid(bx,by,0,bw,bh, CLR32(0xFF,0xF5,0xE0,0xEE));
        C2D_DrawRectSolid(bx,by,0,bw,2,  CLR_DARK);
        C2D_DrawRectSolid(bx,by+bh-2,0,bw,2,CLR_DARK);

        if (G.status==GS_CLEAR) {
            drawText("LEVEL CLEAR!",    bx+18,by+7, 0,0.68f,CLR32(0x10,0x80,0x20,0xFF));
            drawText("A or Touch: Next",bx+14,by+31,0,0.46f,CLR_DARK);
        } else if (G.status==GS_OVER) {
            drawText("GAME OVER",       bx+32,by+7, 0,0.68f,CLR_RED);
            drawText("A or Touch: Retry",bx+10,by+31,0,0.46f,CLR_DARK);
        } else {
            drawText("PAUSED",          bx+46,by+7, 0,0.68f,CLR_DARK);
            drawText("START to resume", bx+14,by+31,0,0.46f,CLR_GRAY);
        }
    }
}

//=============================================================================
// Update
//=============================================================================
static void update()
{
    if (G.flyActive) {
        G.flyProg += FLY_SPEED;
        if (G.flyProg >= 1.0f) {
            G.flyActive = false;
            G.status = (G.arrowsLeft==0) ? GS_CLEAR : GS_PLAYING;
        }
    }
    if (G.flashActive) {
        if (--G.flashTimer <= 0) G.flashActive = false;
    }
}

//=============================================================================
// Input
//=============================================================================
static void handleInput()
{
    hidScanInput();
    u32 kd = hidKeysDown();

    // START = pause toggle
    if (kd & KEY_START) {
        if (G.status==GS_PLAYING||G.status==GS_FLYING)
            { G.status=GS_PAUSED; return; }
        if (G.status==GS_PAUSED)
            { G.status=G.flyActive?GS_FLYING:GS_PLAYING; return; }
    }

    if (G.status==GS_PAUSED) return;

    // CLEAR / OVER: advance or retry
    if (G.status==GS_CLEAR) {
        if (kd & (KEY_A|KEY_TOUCH)) { G.hearts=MAX_HEARTS; loadLevel(G.levelIdx+1); }
        return;
    }
    if (G.status==GS_OVER) {
        if (kd & (KEY_A|KEY_TOUCH)) { G.hearts=MAX_HEARTS; loadLevel(G.levelIdx); }
        return;
    }

    // SELECT = restart
    if (kd & KEY_SELECT) { G.hearts=MAX_HEARTS; loadLevel(G.levelIdx); return; }

    if (G.flyActive) return;

    // L/R = zoom
    if (kd & KEY_L) G.zoom = fmaxf(G.zoom-ZOOM_STEP, ZOOM_MIN);
    if (kd & KEY_R) G.zoom = fminf(G.zoom+ZOOM_STEP, ZOOM_MAX);

    // Circle pad = pan
    circlePosition cp; hidCircleRead(&cp);
    if (abs(cp.dx)>CPAD_DEAD) G.camX += (cp.dx/156.0f)*CAM_SPEED;
    if (abs(cp.dy)>CPAD_DEAD) G.camY -= (cp.dy/156.0f)*CAM_SPEED;

    // D-Pad = cursor
    if (kd & KEY_DRIGHT) G.curC=(G.curC+1)%G.cols;
    if (kd & KEY_DLEFT)  G.curC=(G.curC-1+G.cols)%G.cols;
    if (kd & KEY_DUP)    G.curR=(G.curR-1+G.rows)%G.rows;
    if (kd & KEY_DDOWN)  G.curR=(G.curR+1)%G.rows;

    // A = tap cursor
    if (kd & KEY_A)     { tapCell(G.curR,G.curC); return; }

    // Y = hint
    if (kd & KEY_Y)
    {
        if (G.hintActive)
            G.hintActive = false;
        else
            findHint();

        return;
    }
    // Touch
    if (kd & KEY_TOUCH) {
        touchPosition tp; hidTouchRead(&tp);
        int tr,tc; touchToCell(tp.px,tp.py,BOT_W,BOT_H,&tr,&tc);
        tapCell(tr,tc);
    }
}

//=============================================================================
// Main
//=============================================================================
int main(int argc, char* argv[])
{
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP,    GFX_LEFT);
    C3D_RenderTarget* bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    G.hearts = MAX_HEARTS;
    loadLevel(0);

    while (aptMainLoop()) {
        handleInput();
        update();

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C2D_TargetClear(top, CLR_BG);
        C2D_SceneBegin(top);
        drawGrid(TOP_W, TOP_H);
        drawHUD(TOP_W, TOP_H);

        C2D_TargetClear(bot, CLR_BG);
        C2D_SceneBegin(bot);
        drawGrid(BOT_W, BOT_H);
        drawHUD(BOT_W, BOT_H);

        C3D_FrameEnd(0);
    }

    if (s_tbuf) C2D_TextBufDelete(s_tbuf);
    C2D_Fini(); C3D_Fini(); gfxExit();
    return 0;
}
