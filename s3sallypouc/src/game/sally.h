// S3 SALLY POUC — one sally. Carry the pouch across. Then it is done.
#pragma once
#include "console/gfx.h"
#include "console/system.h"

namespace sally {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_WOOD = 2,
    PAL_POUCH = 3,
    PAL_HERO = 4,
    PAL_IRON = 5,
    PAL_DAWN = 6,
    PAL_ALERT = 7,
    PAL_GO = 8
};

struct Art {
    gs::Mipped hero, pouch, block, crate, gate, bars;
    gs::Mipped glyph[96];
    int font[96] = {};
};

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SALLY POUC"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    // 0 title, 1 the sally, 2 past the yard, 3 the pouch crossed, 4 the sally failed
    int marker() const;
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Play, Pause, Won, Lost };

    void buildArt();
    void begin();
    void finish(bool crossed, const char* why);
    void bot(bool& left, bool& right, bool& jump);
    void stepPlay(bool left, bool right, bool jump);
    bool groundAt(float x) const;
    bool onCrate(float x, float feet) const;
    void draw();
    void spr(const gs::Mipped& m, float cx, float foot, float h, int pal, bool feet, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool onGround_ = true;
    int face_ = 1;
    int plant_ = 0;
    const char* reason_ = "";
    float px_ = 0, py_ = 0, vy_ = 0;
    float cam_ = 0, t_ = 0, watch_ = 0;
};

}  // namespace sally
