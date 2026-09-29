// S3 RICKSHAW PASS — pedal the pass before the storm clock closes it.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace pass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RICKSHAW PASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    float clockLeft() const;
    const char* why() const { return why_; }
    float distance() const { return float(s_); }
    float lateral() const { return lat_; }
    // 0 title, 1 climb, 2 rock band, 3 crest, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void showTitle();
    void startRun();
    void pilot(float& pedal, float& brake, float& steer);
    void stepRun(float pedal, float brake, float steer);
    void fail(const char* why);
    void win();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, int fog);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    double s_ = 0;
    float lat_ = 0;
    float v_ = 0;
    double t_ = 0;
    float time_ = 0;
    float shake_ = 0;
    float wind_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
};

}  // namespace pass
