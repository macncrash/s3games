// S3 HOOP — first to twenty-one. The rim is the judge.
#pragma once

#include <string>

#include "console/system.h"
#include "game/art.h"

namespace hoop {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HOOP"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int lane() const { return lane_; }

private:
    enum class Mode { Title, Aim, Flight, Call, Win, Lose, Pause };
    enum class Call { Swish, Count, Bank, Rim, Short, Long, Air };

    struct Ball {
        float x = 0, y = 0, z = 0;
        float vx = 0, vy = 0, vz = 0;
    };

    void beginMatch();
    void beginAim();
    void launch(float meter);
    void stepBall(float dt);
    void fly(float dt);
    void finishShot();
    void afterCall();
    Call classify() const;
    void aimControl(float dt);
    void autoRelease(float dt);
    void dribble(float dt);
    void backdrop();
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip = false, bool shadow = false);
    void tone(int ch, float freq, float vol, float hold);
    void pumpAudio();
    int worth() const;
    bool shootPressed() const;
    bool startPressed() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Call call_ = Call::Swish;
    Ball ball_{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool youTurn_ = true;
    bool scored_ = false;
    bool rimHit_ = false;
    bool bank_ = false;
    bool crossed_ = false;
    bool rimSnd_ = false;
    bool bankSnd_ = false;
    int you_ = 0;
    int lane_ = 0;
    int shotPts_ = 2;
    int laneIx_ = 0;
    int fanStep_ = -1;
    float feetX_ = 4.15f;
    float aimZ_ = 0;
    float meter_ = 0;
    float meterDir_ = 1;
    float t_ = 0;
    float flight_ = 0;
    float scoreAt_ = 0;
    float callT_ = 0;
    float flash_ = 0;
    float crossPast_ = 0;
    float toneUntil_[3] = {};
    float prevDrib_ = 0;
};

}  // namespace hoop
