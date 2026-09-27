#pragma once

#include "console/system.h"
#include "game/art.h"

namespace golfmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GOLFMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }
    int strokes() const { return strokes_; }
    int holeNo() const { return 1; }

private:
    enum class Mode { Title, Aim, Fly, Win };

    struct Ball {
        float x = kTeeX;
        float y = 0;
        float vx = 0;
        float vy = 0;
        bool live = false;
        bool holed = false;
    };

    void begin();
    void launch(float angDeg, float spd);
    bool stepBall(Ball& b, float dt) const;
    bool flyUntil(Ball b, float ang, float spd, Ball& out) const;
    void pickShot();
    void settle();
    void finishMark();
    void draw();
    void backdrop();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Ball ball_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finished_ = false;
    bool charging_ = false;
    bool planned_ = false;
    int strokes_ = 0;
    int hold_ = 0;
    float aim_ = 34.f;
    float meter_ = 0.f;
    float meterDir_ = 1.f;
    float shotSpd_ = 0.f;
    float botAng_ = 34.f;
    float botSpd_ = 280.f;
    float t_ = 0.f;
};

}  // namespace golfmark
