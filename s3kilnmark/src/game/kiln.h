// S3 KILNMARK — throw, stamp, and fire until the finished mark leaves the kiln.
#pragma once
#include <string>

#include "console/gfx.h"
#include "console/system.h"

namespace kiln {

enum Pal {
    PAL_HUD = 0,
    PAL_CLAY = 1,
    PAL_GLAZE = 2,
    PAL_BRICK = 3,
    PAL_FIRE = 4,
    PAL_WOOD = 5,
    PAL_INK = 6,
    PAL_FIGURE = 7
};

struct Art {
    gs::Mipped kiln, door, glow, wheel, pot, potWide, mark, cone, coneBent;
    gs::Mipped shelf, figure, stamp, smoke;
    gs::Mipped glyph[96];
    int font[96] = {};
};

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KILNMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int heat() const { return int(heat_); }

private:
    enum class Mode { Title, Throw, Stamp, Fire, Leave, Over };

    void buildArt();
    void backdrop();
    void update(float dt);
    void draw();
    void hud(int col, int row, const std::string& s);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void blip(float freq);
    void fanfare();
    float rnd();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool marked_ = false;
    uint32_t rng_ = 0x4b494c4eu;

    float t_ = 0;
    float height_ = 0;
    float wobble_ = 0;
    float spin_ = 0;
    float cx_ = 160, cy_ = 70;
    float tx_ = 188, ty_ = 128;
    float heat_ = 420;
    float damper_ = 0.2f;
    float cone_ = 0;
    float leave_ = 0;
    int smudge_ = 0;
    int cracks_ = 0;
    float msgT_ = 0;
    std::string msg_;
};

}  // namespace kiln
