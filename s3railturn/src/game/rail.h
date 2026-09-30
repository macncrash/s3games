// S3 RAIL TURN — one rail, three turns, do not tip.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace railturn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RAIL TURN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int turns() const { return cleared_; }
    int lives() const { return lives_; }

private:
    enum class Mode { Title, Run, Tip, Win, Lose };

    void resetRun();
    void update(float dt);
    void draw();
    void text(const std::string& s, float x, float y, int pal, bool center);
    void spr(const gs::Image& img, float cx, float cy, float h, int pal, int fog = 0, bool shadow = false);
    float kappa(float s) const;
    float lateral(float dist) const;
    float required() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int lives_ = 3;
    int cleared_ = 0;
    float s_ = 0;
    float speed_ = 20;
    float bank_ = 0;
    float lean_ = 0;
    float slipT_ = 0;
    float t_ = 0;
    float hold_ = 0;
    float shake_ = 0;
    int cp_ = 0;
};

}  // namespace railturn
