// S3 RAILMARK — take the rail and set down on the mark.
// The clock is the other crew.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace rail {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RAILMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float clock() const { return clock_; }
    float crewLeft() const { return crewLeft_; }

private:
    enum class Mode { Title, Run, Set, Over };

    void beginRun();
    void update(float dt);
    void physics(float dt, bool gas, bool brake);
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void blip(float freq);
    void fanfare();
    float brakeStop() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    float t_ = 0;
    float x_ = 40;
    float vx_ = 0;
    float clock_ = 0;
    float crewLeft_ = 0;
    float still_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    float setT_ = 0;
    int fanStep_ = -1;
    float fanT_ = 0;
    bool engineOn_ = false;
    const char* fail_ = "MISSED THE MARK";
};

}  // namespace rail
