// S3 BIKE — one kilometer. Don't touch wheels.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace bike {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BIKE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int meters() const { return meters_; }
    const char* result() const { return result_; }

private:
    enum class Mode { Title, Run, Pause, Crash, Fail, Win };
    enum class Kind { Ground, High };

    struct Wheel {
        float z;
        Kind kind;
    };
    struct In {
        bool pedal, brake, hop, duck;
    };

    void buildCourse();
    void resetRun();
    In controls();
    void runLogic(float dt);
    void collide();
    void finishWin();
    void finishFail();
    void mix();
    void draw();
    void backdrop();
    void hud();
    void blit(const gs::Mipped& m, float x, float y, int pal, bool flip = false);
    void image(const gs::Image& img, float x, float y, int pal);
    bool go() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    std::vector<Wheel> wheels_;
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Run;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool grounded_ = true;
    bool duck_ = false;
    bool danger_ = false;
    bool burst_ = false;
    int meters_ = 0;
    int hopBuf_ = 0;
    char result_[180] = {};

    float t_ = 0;
    float raceT_ = 0;
    float modeT_ = 0;
    float odo_ = 0;
    float speed_ = 10;
    float lift_ = 0;
    float vy_ = 0;
    float jumpSnd_ = 0;
    float winT_ = 0;
    int shake_ = 0;
};

}  // namespace bike
