// S3 BOCCE — first to seven. Closest to the little ball.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace bocce {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BOCCE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }
    int ends() const { return ends_; }
    std::string dump() const;

    struct Ball {
        float x = 0, y = 0, vx = 0, vy = 0;
        float wantX = -1, wantY = -1;
        int side = 0;
        bool live = true;
        bool rest = true;
    };

private:
    enum class Mode { Title, Aim, Roll, Reject, Score, Pause, Win, Lose };
    enum class Phase { Jack, Bowl };

    void beginEnd();
    void layCourt();
    void botLaunch();
    void cpuLaunch();
    void humanAim(const gs::Pad& pad);
    void spawn(int side, float x, float y, float vx, float vy, float wantX, float wantY);
    void rollTo(int side, float tx, float ty);
    void stepSim();
    void forceRest();
    bool allRest() const;
    void onRest();
    void removeJack();
    void applyScore();
    void chime(int kind);
    void quiet();
    void draw();
    void backdrop();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void ghost(float& gx, float& gy) const;
    void drawBall(int side, float x, float y);
    bool yourTurn() const;
    Ball* findJack();
    const Ball* findJack() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Phase phase_ = Phase::Jack;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool charging_ = false;
    int you_ = 0;
    int them_ = 0;
    int ends_ = 0;
    int endNo_ = 1;
    int next_ = 0;
    int thrown_[2] = {};
    int gainYou_ = 0;
    int gainThem_ = 0;
    float aimY_ = 112.f;
    float aimAng_ = 0;
    float meter_ = 0;
    float meterDir_ = 1.f;
    float t_ = 0;
    float aimT_ = 0;
    float rollT_ = 0;
    float sayT_ = 0;
    float toneT_ = 0;
    float hitCd_ = 0;
    std::string say_;
    std::vector<Ball> balls_;
};

}  // namespace bocce
