// S3 CRANESLIP — set the barge in the slip before the tide covers the coping.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace slip {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CRANESLIP"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float tide() const { return clock_; }

private:
    enum class Mode { Title, Play, Win, Fail };
    enum class Job { ToBarge, LowerGrab, Raise, ToSlip, LowerDrop };

    void begin();
    void update(float dt);
    void botPlan(float& ax, float& hoist, bool& act);
    void draw();
    bool grab();
    void release();
    void hookAt(float& x, float& y) const;
    bool inSlip(float x) const;
    bool startPressed() const;
    bool actPressed() const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void blip(float freq);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Job job_ = Job::ToBarge;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool held_ = false;
    bool actLatch_ = false;
    float tx_ = 80.f;
    float vx_ = 0.f;
    float len_ = 30.f;
    float bx_ = 108.f;
    float by_ = 176.f;
    float bvx_ = 0.f;
    float clock_ = 42.f;
    float water_ = 186.f;
    float t_ = 0.f;
    float melodyT_ = 0.f;
    int melody_ = -1;
};

}  // namespace slip
