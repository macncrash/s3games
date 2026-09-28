// S3 SHUFFLECHIME — Shuffle: the hour has to chime.
// A disk that stops in the three before twelve is lifted.
// It has to be resting in the score as the hour strikes. Three disks, then the hour is gone.
#pragma once

#include "console/gfx.h"
#include "console/system.h"

namespace shufflechime {

constexpr int kGraceSec = 18;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SHUFFLECHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return solved_; }
    bool inScore() const;
    bool sliding() const;
    int disks() const { return thrown_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Aim, Roll, Again, Chime, Fail, Over, Pause };

    struct Disk {
        float x = 0, y = 0, vx = 0, vy = 0;
        bool dead = false;
        bool live = false;
    };
    struct End {
        float x = 0, y = 0;
        int frames = 0;
        bool dead = false;
        bool score = false;
    };

    void buildArt();
    void layTable();
    void showTitle();
    void newGame();
    void solve();
    End coast(float x, float speed) const;
    void launch();
    void shove(Disk& d) const;
    void settle();
    void beginAgain(const char* why);
    void beginChime();
    void beginFail(const char* why);
    const char* lieName() const;

    int clockSec() const;
    int framesUntilHour() const;
    bool onHour() const;
    bool pastHour() const;
    void split(int& h, int& m, int& s) const;
    void faceTime(int& h, int& m, int& s) const;

    void draw();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void hand(float ang, float len, int n, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Disk disk_{};
    End preview_{};
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool solved_ = false;
    bool wasWindow_ = false;
    int thrown_ = 0;
    int playFrames_ = 0;
    int titleFrames_ = 0;
    int againFrames_ = 0;
    int chimeFrames_ = 0;
    int failFrames_ = 0;
    int strikes_ = 0;
    int runFrames_ = 0;
    float solSpeed_ = 0;
    float aimX_ = 160.f;
    float aimSpeed_ = 0;
    float bellPh_ = 0;
    float anim_ = 0;

    gs::Mipped diskImg_;
    gs::Mipped shadow_;
    gs::Mipped chev_;
    gs::Mipped bell_;
    gs::Image dot_;
    gs::Image face_;
    int font_[96] = {};
    int wax_ = 1, line_ = 1, z3_ = 1, z2_ = 1, z1_ = 1;
    int railH_ = 1, railV_ = 1, wood_ = 1, gutter_ = 1;
};

}  // namespace shufflechime
