// S3 CRANE — three crates, ship them to the mark. A drop fails the shift.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace crane {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CRANE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int shipped() const { return shipped_; }
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Shift, Drop, Fail, Win };
    enum class Box { Wait, Carry, Ship, Fall };
    enum class Job { Seek, Lower, Lift, Travel, Descend, Done };

    struct Crate {
        float x = 0, y = 0, vy = 0;
        Box box = Box::Wait;
    };

    void begin();
    void titlePose();
    void update(float dt);
    void draw();
    void botPlan(float hx, float& ax, float& hoist, bool& act);
    bool grab();
    void release();
    void failDrop(const char* banner);
    void hookAt(float& x, float& y) const;
    void carryAt(float& x, float& y) const;
    int nextBox() const;
    bool startPressed() const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);
    void blip(float freq);
    void chime(float dt);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Job job_ = Job::Seek;
    Crate crates_[3]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool resting_ = false;
    bool onMark_ = false;
    bool onDock_ = false;
    bool onShip_ = false;
    int carrying_ = -1;
    int shipped_ = 0;
    int melody_ = -1;
    float tx_ = 80.f, vx_ = 0.f;
    float len_ = 40.f;
    float th_ = 0.f, om_ = 0.f;
    float t_ = 0.f, shiftT_ = 0.f, phaseT_ = 0.f, dropT_ = 0.f, splashT_ = 0.f;
    float melodyT_ = 0.f;
    float splashX_ = 0.f;
    const char* why_ = "the shift ran out";
    const char* banner_ = "";
};

}  // namespace crane
