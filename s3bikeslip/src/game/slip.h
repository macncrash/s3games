// S3 BIKE SLIP — berth in the slip before the tide turns.
// Missing the end of the slip fails the leg.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace slip {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BIKE SLIP"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* result() const { return result_; }

private:
    enum class Mode { Title, Ride, Pause, Win, Fail };

    struct In {
        float steer;
        bool pedal;
        bool brake;
        bool start;
    };

    void resetRide();
    In controls() const;
    void logic(float dt);
    void finishWin();
    void finishFail(const char* why);
    void audio();
    void draw();
    void skyRoad();
    void sprites();
    void hud();
    void blit(const gs::Mipped& m, float x, float y, float h, int pal, int fog = 0, bool flip = false);
    void image(const gs::Image& img, float x, float y, int pal);
    float laneAt(float z) const;
    float halfAt(float z) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool chainOn_ = false;
    int hold_ = 0;
    int why_ = 0;
    char result_[200] = {};

    float t_ = 0;
    float rideT_ = 0;
    float clock_ = 0;
    float z_ = 0;
    float x_ = 0;
    float vx_ = 0;
    float speed_ = 0;
    float steer_ = 0;
    int hor_ = 96;
};

}  // namespace slip
