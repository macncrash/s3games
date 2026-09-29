// S3 LOOMBELL — a short loom. The bell rings before the third try dies.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace loombell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LOOMBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    int dead() const { return dead_; }
    int tryNo() const { return attempt_ + 1; }
    int attempt() const { return attempt_; }
    const char* why() const { return why_; }

private:
    enum class Mode { Title, Try, Rise, Gap, Over };

    void begin();
    void cast(bool hit);
    void killTry();
    void spr(const gs::Image& img, float cx, float cy, int pal, bool hflip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void sky();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    int dead_ = 0;
    int attempt_ = 0;
    int age_ = 0;
    int anim_ = 0;
    int dir_ = 1;
    const char* why_ = "";
};

}  // namespace loombell
