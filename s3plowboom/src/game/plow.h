// S3 PLOW BOOM — the plow has one job: deliver the drive to the boom.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace plowboom {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PLOW BOOM"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return you_; }
    const char* why() const { return why_; }

private:
    enum class Mode { Title, Drive, Pause, Win, Fail };

    struct Flake {
        float x, y, life;
    };

    void begin();
    void controls(float& gas, float& steer);
    void pilot(float& gas, float& steer);
    void physics(float dt, float gas, float steer);
    bool driveInside() const;
    bool plowHitsBoom() const;
    bool driveHitsHead() const;
    void audio(float dt, float gas);
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal);
    void worldToScreen(float wx, float wy, float& sx, float& sy) const;
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int face() const;
    void blip(float freq);
    void fail(const char* why);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int fanStep_ = -1;
    float you_ = 0;
    float storm_ = 0;
    float hold_ = 0;
    float still_ = 0;
    float t_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0, speed_ = 0;
    float blade_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 2.2f;
    float tone_ = 0;
    char why_[64] = {};
    Flake flake_[14]{};
    int flakeN_ = 0;
    bool seated_ = false;
    float seatX_ = 0, seatY_ = 0, seatH_ = 0;
};

}  // namespace plowboom
