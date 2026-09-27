// S3 BARGE PASS — clear the gorge before the storm clock.
// A hull on the rock fails the job. The clock dying short of the arch fails it too.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace bargepass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BARGE PASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    float stormLeft() const { return storm_; }
    const char* why() const { return why_; }
    // 0 title, 1 the wide water, 2 the pinch, 3 the mouth, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void begin();
    void pilot(float& thrust, float& steer);
    void stepRun(float thrust, float steer);
    void wreck(const char* why);
    void finish();
    void blip(float freq);
    void draw();
    void quad(const gs::Mipped& m, float x0, float y0, float x1, float y1, int pal);
    void mark(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    float centerAt(float x) const;
    float halfAt(float x) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    float time_ = 0;
    float storm_ = 0;
    float x_ = 0, y_ = 0, vx_ = 0, vy_ = 0;
    float cam_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
};

}  // namespace bargepass
