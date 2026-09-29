// S3 HEADER BOOM — the header has one job: deliver the drive to the boom.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace headerboom {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HEADER BOOM"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return t_; }
    const char* why() const { return why_ ? why_ : ""; }

private:
    enum class Mode { Title, Run, Fail, Win };

    void begin();
    void stepRun();
    void botInput(float& thrust, bool& release);
    void fail(const char* why);
    void succeed();
    void blip(float freq);
    void paintSky();
    void paintWorld();
    void spr(const gs::Mipped& m, float wx, float wy, float destH, int pal, bool flip = false);
    void text(const char* s, float x, float y, float scale, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool released_ = false;
    bool supported_ = false;
    float t_ = 0;
    float bow_ = 0;
    float bv_ = 0;
    float dx_ = 0, dy_ = 0, dvx_ = 0, dvy_ = 0;
    float still_ = 0;
    float flash_ = 0;
    const char* why_ = "";
    const char* banner_ = "";
};

}  // namespace headerboom
