// S3 CLIFFBUOY — round three buoys to port and return to the same dock.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace cliffbuoy {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CLIFFBUOY"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Run, Fail, Win };

    void begin();
    void update(float dt);
    void draw();
    void botDrive(float& steer, float& throttle, float& brake);
    void steerOf(float& steer, float& throttle, float& brake) const;
    bool startPressed() const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);
    void blip(float freq);
    void chime(float dt);
    int headingFrame() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool cleared_[3] = {};
    bool armed_ = false;
    int next_ = 0;
    int wp_ = 0;
    int melody_ = -1;
    float x_ = 0, y_ = 0, heading_ = 0, speed_ = 0;
    float sweep_ = 0, lastAng_ = 0;
    float t_ = 0, legT_ = 0, hold_ = 0, melodyT_ = 0;
    const char* why_ = "the leg ran out";
};

}  // namespace cliffbuoy
