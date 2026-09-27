#pragma once

#include "console/system.h"
#include "game/art.h"

namespace tablemark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TABLEMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }
    int shots() const { return shots_; }

private:
    enum class Mode { Title, Play, Win };

    void begin();
    void finishMark();
    void drive(float dt);
    void physics(float dt);
    void hit(float& mx, float& my, float& mvx, float& mvy, bool you);
    void aimShot();
    void draw();
    void backdrop();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finished_ = false;
    bool slap_ = false;
    int shots_ = 0;
    int hold_ = 0;
    float px_ = 160.f, py_ = 128.f, pvx_ = 0.f, pvy_ = 0.f;
    float yx_ = 160.f, yy_ = 164.f, yvx_ = 0.f, yvy_ = 0.f;
    float ox_ = 230.f, oy_ = 86.f, ovx_ = 0.f, ovy_ = 0.f;
    float t_ = 0.f;
};

}  // namespace tablemark
