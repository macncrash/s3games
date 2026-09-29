// S3 SUB LOCK — take the sub through one flooded lock. A scrape on a gate fails the job.
// The other crew is the clock: if they clear first, the lock is theirs.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace sublock {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SUB LOCK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    const char* why() const { return why_; }
    // 0 title, 1 approach, 2 in the chamber, 3 clear of the upper gate, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };
    enum class Phase { Approach, Shut, Fill, Open, Leave };

    void begin();
    void pilot(float& thrust, float& ballast);
    void step(float thrust, float ballast);
    void scrape(const char* why);
    void finish();
    void blip(float freq);
    void draw();
    void quad(const gs::Mipped& m, float x0, float y0, float x1, float y1, int pal);
    void mark(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    float bow() const;
    float stern() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Approach;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    float time_ = 0;
    float x_ = 180.f, y_ = 0, vx_ = 0, vy_ = 0;
    float lo_ = 1, hi_ = 0, fill_ = 0;
    float crew_ = 0;
    float cam_ = 180.f;
    float shake_ = 0;
    float beep_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
    float prop_ = 0;
};

}  // namespace sublock
