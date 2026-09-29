// S3 RICKSHAW LOCK — pedal through one lock. A scrape on a gate fails the pass.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace ricklock {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RICKSHAW LOCK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    const char* why() const { return why_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return psi_; }
    float speed() const { return v_; }
    // 0 title, 1 approach, 2 in the chamber, 3 clear of the upper gate, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };
    enum class Phase { Approach, Shut, Rise, Open, Leave };

    void begin();
    void pilot(float& pedal, float& brake, float& steer);
    void step(float pedal, float brake, float steer);
    void scrape(const char* why);
    void finish();
    void blip(float freq);
    void draw();
    void blit(const gs::Mipped& m, float wx, float wy, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    bool bodyInside() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Approach;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    float time_ = 0;
    float x_ = 0, y_ = 0, psi_ = 0, v_ = 0;
    float lo_ = 1, hi_ = 0, fill_ = 0;
    float cam_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    float spin_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
};

}  // namespace ricklock
