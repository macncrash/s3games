// S3 SCULL KILO — finish the kilometer without touching wheels.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace kilo {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SCULL KILO"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int meters() const { return meters_; }

private:
    enum class Mode { Title, Row, Fail, Win };

    void begin();
    void row(float dt);
    void botStick(float& steer, float& power) const;
    void paintSky();
    void paintWater();
    void paintWorld();
    void paintHull();
    void glyphText(const char* s, int x, int y, int scale, int pal);
    void project(float wx, float wz, float& sx, float& sy, float& ppm) const;
    void spr(const gs::Mipped& m, float cx, float footY, float destH, int pal, bool flip);
    void fail(const char* why);
    void blip(float freq);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int meters_ = 0;
    float t_ = 0;
    float x_ = 0;
    float vx_ = 0;
    float pz_ = 0;
    float phase_ = 0;
    float flash_ = 0;
    const char* why_ = "";
};

}  // namespace kilo
