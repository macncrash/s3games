// S3 BOCCECHIME — a short bocce. Leave when the hour has to chime.
// The pallino sits in a gold ring at the far boards. A bowl chimes only if it
// dies inside that ring while the clock is on twelve. Early gold is not the
// hour. Three bowls, then the hour is gone.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace boccechime {

constexpr int kFpc = 6;
constexpr int kGraceSec = 30;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 90;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BOCCECHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool onTheHour() const { return onHour(); }
    int bowls() const { return used_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Aim, Roll, Early, Dead, Chime, Leave, Fail, Over };

    struct Bowl {
        float x = 0, y = 0, vx = 0, vy = 0;
        bool live = false;
        bool moving = false;
        bool burned = false;
    };

    void toTitle();
    void begin();
    void launch();
    void coast(float dt);
    void settle();
    void beginChime();
    void miss(const char* why);
    void fail(const char* why);

    int secAt(int frames) const;
    int clockSec() const;
    bool onHour() const;
    bool pastHour() const;
    void split(int& h, int& m, int& s) const;

    void draw();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void clockLine();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Bowl bowl_[3]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool clockOn_ = false;
    int used_ = 0;
    int dead_ = 0;
    int hold_ = 0;
    int playFrames_ = 0;
    int strikes_ = 0;
    int chimeFrames_ = 0;
    const char* why_ = "";
    float aim_ = 0.f;
    float power_ = 0.5f;
    float powerDir_ = 1.f;
    float anim_ = 0.f;
};

}  // namespace boccechime
