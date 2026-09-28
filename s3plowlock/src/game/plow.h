// S3 PLOW LOCK — pass the lock without scraping a gate. Missing the end fails the leg.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace plowlock {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PLOW LOCK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    const char* why() const { return why_; }
    // 0 title, 1 approach, 2 chamber, 3 clear of the upper gate, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };
    enum class Phase { Approach, Shut, Hold, Open, Leave };

    void begin();
    void pilot(float& gas, float& steer);
    void step(float gas, float steer);
    void fail(const char* why);
    void finish();
    void blip(float freq);
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal);
    void quad(const gs::Mipped& m, float x0, float y0, float x1, float y1, int pal);
    void worldToScreen(float wx, float wy, float& sx, float& sy) const;
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    float bowY() const;
    float sternY() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Approach;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    float time_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0, speed_ = 0;
    float lo_ = 1, hi_ = 0, hold_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 2.1f;
    float shake_ = 0;
    float tone_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
};

}  // namespace plowlock
