// S3 CURLCHIME — Curl: the hour has to chime.
// A draw that covers the button while the clock is still short of twelve is lifted.
// The stone has to stop on the button as the hour strikes. Three stones, then the hour is gone.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace curlchime {

constexpr int kGraceSec = 24;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CURLCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return solved_; }
    bool onButton() const;
    bool sliding() const;
    int stones() const { return thrown_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Aim, Slide, Again, Chime, Fail, Over, Pause };

    struct Stone {
        float x = 0, y = 0, vx = 0, vy = 0;
        int handle = 1;
        bool dead = false;
        bool live = false;
    };
    struct End {
        float x = 0, y = 0;
        int frames = 0;
        bool dead = false;
        bool button = false;
        bool hog = false;
    };
    struct Puff {
        float x = 0, y = 0, life = 0;
    };

    void showTitle();
    void newGame();
    void solve();
    End coast(float vx, float vy, int handle, float sweep, bool record);
    void launch();
    void shove(Stone& r, float sweep, float dt) const;
    void settle();
    void beginAgain(const char* why);
    void beginChime();
    void beginFail(const char* why);
    const char* lieName(const Stone& s) const;

    int clockSec() const;
    int framesUntilHour() const;
    bool onHour() const;
    bool pastHour() const;
    void split(int& h, int& m, int& s) const;
    void faceTime(int& h, int& m, int& s) const;

    void blip(float freq);
    void strikeBell();
    void tickSound();
    void decayAudio();

    void draw();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip = false,
             bool shadow = false);
    void sprM(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void clockAt(float cx, float cy, float size, bool live);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Stone stone_{};
    End preview_{};
    Puff puff_[6];
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool solved_ = false;
    bool wasWindow_ = false;
    int thrown_ = 0;
    int playFrames_ = 0;
    int titleFrames_ = 0;
    int slideFrames_ = 0;
    int againFrames_ = 0;
    int chimeFrames_ = 0;
    int failFrames_ = 0;
    int runFrames_ = 0;
    int lastSec_ = -1;
    int beep_ = 0;
    int strikes_ = 0;
    int solHandle_ = 1;
    int pathN_ = 0;
    float solVx_ = 0, solVy_ = 12.f, solDist_ = 99.f;
    float aimVx_ = 0, aimVy_ = 12.f;
    int handle_ = 1;
    float sweep_ = 0;
    float anim_ = 0;
    float bellPh_ = 0;
    float pathX_[16] = {};
    float pathY_[16] = {};
    float lieX_ = 0, lieY_ = 0;
};

}  // namespace curlchime
