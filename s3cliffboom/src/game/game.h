// S3 CLIFF BOOM — deliver the drive onto the boom. Missing the end fails the leg.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace cliffboom {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CLIFF BOOM"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return legT_; }
    const char* why() const { return why_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const { return speed_; }
    int phase() const { return phase_; }
    // 0 title, 1 on the leg, 2 in the pocket, 3 holding, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Fail, Win };

    void begin();
    void update(float dt);
    void draw();
    void botDrive(float& steer, float& throttle, float& brake);
    void steerOf(float& steer, float& throttle, float& brake) const;
    bool startPressed() const;
    bool inPocket() const;
    void fail(const char* why);
    void succeed();
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
    int phase_ = 0;
    int wp_ = 0;
    int melody_ = -1;
    float x_ = 0, y_ = 0, heading_ = 0, speed_ = 0;
    float t_ = 0, legT_ = 0, hold_ = 0, melodyT_ = 0;
    float camX_ = 0, camY_ = 0;
    const char* why_ = "the leg ran out";
};

}  // namespace cliffboom
