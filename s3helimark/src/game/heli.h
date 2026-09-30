// S3 HELIMARK — the heli has one job: set down on the mark.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace heli {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HELIMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }

private:
    enum class Mode { Title, Fly, Boom, Down };

    void resetCraft();
    void pilot(bool& left, bool& right, bool& lift);
    void physics(float dt, bool left, bool right, bool lift);
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int lives_ = 3;
    float t_ = 0;
    float x_ = 48, y_ = 64;
    float vx_ = 0, vy_ = 0;
    float boom_ = 0;
    float fan_ = 0;
    int fanStep_ = -1;
    bool lift_ = false;
    bool faceL_ = false;
};

}  // namespace heli
