// S3 MUSHBOX — the mush has one job: stop inside the box.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace mushbox {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MUSHBOX"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int boxes() const { return cleared_; }
    int stage() const { return stage_; }
    // 0 title, 1 sliding, 2 stopped in the box, 3 ended
    int marker() const;

private:
    enum class Mode { Title, Slide, Banner, Win, Lose };

    void begin(int stage);
    void physics(float ix, float iy, bool brake);
    void bot(float& ix, float& iy, bool& brake) const;
    bool settled() const;
    bool inBox() const;
    bool offIce() const;
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool shadow = false);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int stage_ = 0;
    int cleared_ = 0;
    int lives_ = 3;
    int still_ = 0;
    int banner_ = 0;
    bool lastIn_ = false;
    float x_ = 0, y_ = 0, vx_ = 0, vy_ = 0;
    float t_ = 0;
    float bx_ = 0, by_ = 0, bw_ = 0, bh_ = 0;
    int note_ = -1;
};

}  // namespace mushbox
