#pragma once

#include "console/system.h"
#include "game/art.h"

namespace boccemark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BOCCEMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }

private:
    enum class Mode { Title, Aim, Roll, Win, Lose };
    enum class Phase { Mark, You, Them };

    struct Ball {
        float x = 0, y = 0, vx = 0, vy = 0;
        int side = 0;
        bool live = true;
    };

    void begin();
    void launch(Ball& b, float tx, float ty);
    void throwNext();
    void stepBalls(float dt);
    void separate();
    bool settled() const;
    void judge();
    void draw();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Mark;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finished_ = false;
    int hold_ = 0;
    float aim_ = 0.f;
    float meter_ = 0.f;
    float meterDir_ = 1.f;
    float t_ = 0.f;
    Ball balls_[5];
    int n_ = 0;
};

}  // namespace boccemark
