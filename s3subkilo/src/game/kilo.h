// S3 SUB KILO — finish one kilometer in the sub. Touching a wheel fails the leg.
// Missing the end gate fails it too.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace subkilo {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SUB KILO"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    // 0 title, 1 the run, 2 a wheel is close, 3 the end gate, 4 decided
    int marker() const;

private:
    enum class Mode { Title, Dive, Fail, Win };

    struct Wheel {
        float m;
        float y;
        float r;
    };

    void beginLeg();
    void update(float dt);
    void draw();
    void steerBot();
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    bool wheelHit(const Wheel& w, float& sx, float& sy) const;
    float screenX(float meters) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool touched_ = false;
    bool missed_ = false;
    float dist_ = 0;
    float subY_ = 120;
    float speed_ = 20;
    float air_ = 80;
    float t_ = 0;
    float hum_ = 0;
    int nearWheel_ = 0;
    std::string banner_;
};

}  // namespace subkilo
