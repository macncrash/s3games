// S3 BOCCE SEVEN — bowl until one side is first to seven, then leave.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace bocceseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BOCCE SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }
    int ends() const { return ends_; }

private:
    enum class Mode { Title, Aim, Roll, Tally, Win };
    enum class Phase { Pallino, Bowls };

    struct Ball {
        float x = 0, y = 0, vx = 0, vy = 0;
        int side = 0;
        bool live = true;
    };

    void openEnd();
    void paintCourt();
    void launch(int side, float tx, float ty);
    void botThrow();
    void human(const gs::Pad& pad);
    void stepBalls();
    bool settled() const;
    void afterRest();
    void scoreEnd();
    void tone(int kind);
    void hush();
    void draw();
    void sky();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    const Ball* pallino() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Pallino;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool charging_ = false;
    int you_ = 0;
    int them_ = 0;
    int ends_ = 0;
    int endNo_ = 1;
    int thrown_ = 0;
    int nextSide_ = 0;
    int gainYou_ = 0;
    int gainThem_ = 0;
    float aimY_ = 112.f;
    float aimNudge_ = 0.f;
    float meter_ = 0.35f;
    float meterDir_ = 1.f;
    float clock_ = 0;
    float toneT_ = 0;
    std::string say_;
    std::vector<Ball> balls_;
};

}  // namespace bocceseven
