// S3 HELIPLAT — stop level with the platform.
// Skids on the bay mark, nose flat, and still. Missing the end fails the leg.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace heliplat {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HELIPLAT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    const char* why() const { return why_ ? why_ : ""; }
    float x() const { return x_; }
    float alt() const { return y_; }
    float pitch() const { return pitch_; }
    // 0 title, 1 approach, 2 over the platform, 3 holding level, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void begin();
    void pilot(float& thrust, float& climb, float& lean);
    void step(float thrust, float climb, float lean);
    void win();
    void fail(const char* why);
    void blip(float freq);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void quad(float x0, float y0, float x1, float y1, const gs::Mipped& m, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    float time_ = 0;
    float x_ = 0, y_ = 0, vx_ = 0, vy_ = 0;
    float pitch_ = 0, pitchV_ = 0;
    float hold_ = 0, idle_ = 0;
    float cam_ = 0, shake_ = 0, beep_ = 0;
    int rotor_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
};

}  // namespace heliplat
