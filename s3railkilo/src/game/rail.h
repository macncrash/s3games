// S3 RAILKILO — one rail, one kilometer, wheels are a disqualification.
// The other crew is only a clock.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace railkilo {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RAILKILO"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int meters() const { return meters_; }
    const char* result() const { return result_; }

private:
    enum class Mode { Title, Run, Pause, Spill, Fail, Win };
    enum class Kind { Scrap, Hang };

    struct Wheel {
        float z;
        Kind kind;
    };
    struct In {
        bool throttle, brake, hop, duck;
    };

    void layRail();
    void resetRun();
    In controls();
    void runLogic(float dt);
    void collide();
    void finishWin();
    void finishFail(const char* why);
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
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool onRail_ = true;
    bool ducked_ = false;
    bool danger_ = false;
    int meters_ = 0;
    int hopBuf_ = 0;
    int shake_ = 0;
    char result_[220] = {};

    float t_ = 0;
    float raceT_ = 0;
    float modeT_ = 0;
    float odo_ = 0;
    float crew_ = 0;
    float speed_ = 10.f;
    float lift_ = 0;
    float vy_ = 0;
    float hopSnd_ = 0;
};

}  // namespace railkilo
