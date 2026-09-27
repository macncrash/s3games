// S3 GOLFCHIME — Golf: the hour has to chime.
// A putt that finds the cup before twelve is lifted. The ball has to drop
// as the hour strikes. Three balls, then the hour is gone.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace golfchime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GOLFCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return solved_; }
    bool rolling() const { return mode_ == Mode::Roll; }
    int balls() const { return used_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Aim, Roll, Lift, Chime, Fail, Over };

    struct Ball {
        float x = kTeeX;
        float y = kTeeY;
        float vx = 0;
        float vy = 0;
        bool live = false;
        bool holed = false;
    };

    void showTitle();
    void newGame();
    void solve();
    int rollUntil(float ang, float spd, Ball& out) const;
    bool stepBall(Ball& b) const;
    void launch(float ang, float spd);
    void settle();
    void beginLift(const char* why);
    void beginChime();
    void beginFail(const char* why);

    int clockSec() const;
    int framesUntilHour() const;
    bool onHour() const;
    bool pastHour() const;
    void face(int& h, int& m, int& s) const;

    void draw();
    void backdrop();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void handAt(float cx, float cy, float ang, float len);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Ball ball_{};
    Mode mode_ = Mode::Title;
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool solved_ = false;
    bool charging_ = false;
    int used_ = 0;
    int playFrames_ = 0;
    int hold_ = 0;
    int solTravel_ = 0;
    float aim_ = 28.f;
    float meter_ = 0.35f;
    float meterDir_ = 1.f;
    float solAng_ = 28.f;
    float solSpd_ = 160.f;
    float anim_ = 0.f;
};

}  // namespace golfchime
