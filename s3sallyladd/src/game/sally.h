// S3 SALLY LADD — at the sally, reach the far ladder. Miss that and the watch is over.
#pragma once
#include "console/gfx.h"
#include "console/system.h"

namespace sallyladd {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_WOOD = 2,
    PAL_HERO = 3,
    PAL_IRON = 4,
    PAL_DAWN = 5,
    PAL_ALERT = 6,
    PAL_GO = 7
};

struct Art {
    gs::Mipped hero, block, rung, gate, ditch;
    gs::Mipped glyph[96];
    int font[96] = {};
};

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SALLY LADD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    // 0 title, 1 the sally, 2 the far wall, 3 the ladder taken, 4 the watch failed
    int marker() const;
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Play, Pause, Won, Lost };

    void buildArt();
    void begin();
    void finish(bool climbed, const char* why);
    void bot(bool& left, bool& right, bool& jump, bool& up);
    void stepPlay(bool left, bool right, bool jump, bool up);
    int deckAt(float x, float feet) const;
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
    bool climbing_ = false;
    int face_ = 1;
    const char* reason_ = "";
    float px_ = 0, py_ = 0, vy_ = 0;
    float cam_ = 0, t_ = 0, watch_ = 0;
};

}  // namespace sallyladd
