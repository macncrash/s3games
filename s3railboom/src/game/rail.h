// S3 RAILBOOM — take the rail and deliver the drive to the boom.
// The clock is the other crew.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace rail {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RAILBOOM"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float clock() const { return clock_; }
    float crewLeft() const { return crewLeft_; }

private:
    enum class Mode { Title, Run, Deliver, Over };

    void beginRun();
    void update(float dt);
    void physics(float dt, bool gas, bool brake, bool jump, bool duck);
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    bool gapAt(float x) const;
    float respawnBefore(float x) const;
    void blip(float freq);
    void fanfare();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool onRail_ = true;
    bool coupled_ = true;
    bool ducked_ = false;
    int lives_ = 3;
    int strikes_ = 0;
    float t_ = 0;
    float x_ = 80;
    float y_ = 0;
    float vx_ = 0;
    float vy_ = 0;
    float clock_ = 0;
    float crewLeft_ = 0;
    float hitCd_ = 0;
    float recouple_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    float deliver_ = 0;
    int fanStep_ = -1;
    float fanT_ = 0;
    bool engineOn_ = false;
};

}  // namespace rail
