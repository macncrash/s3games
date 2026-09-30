// S3 METRO TURN — three tunnel bends. Bank the cars or they tip.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace metroturn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 METRO TURN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_ ? why_ : ""; }
    float seconds() const { return race_; }
    int turns() const { return turns_; }
    float rollDeg() const { return roll_; }
    float speed() const { return speed_; }

private:
    enum class Mode { Title, Run, Pause, Win, Fail };

    void showTitle();
    void startRun();
    void buildTrack();
    float curveAt(float z) const;
    float trackX(float z) const;
    void controls(float& bank, float& throttle, float& brake);
    void pilot(float& bank, float& throttle, float& brake);
    void physics(float bank, float throttle, float brake, float dt);
    void markTurns(float zPrev);
    void succeed();
    void fail(const char* why);
    const char* tipWhy() const;
    void audio();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void blit(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool made_[3] = {};
    const char* why_ = "";
    int turns_ = 0;
    float t_ = 0;
    float race_ = 0;
    float z_ = 0;
    float speed_ = 0;
    float bank_ = 0;
    float roll_ = 0;
    float rollVel_ = 0;
    float x_[240] = {};
    static constexpr float kStep = 5.f;
    static constexpr int kSamples = 240;
    static constexpr float kTip = 40.f;
};

}  // namespace metroturn
