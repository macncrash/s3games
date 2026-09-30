// S3 DRUMCHIME — play drum until the hour has to chime. Leave when that is true.
// The gold snare is the hour. A hit before twelve spends a stick. Only a gold
// hit inside the hour starts the chime, and only leaving then opens the door.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace drumchime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DRUMCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool onTheHour() const { return onHour(); }
    int hour() const;
    int minute() const;
    int second() const;
    int blows() const { return blows_; }
    int sticks() const { return sticks_; }
    const char* reason() const { return reason_; }
    const char* face() const { return face_; }

private:
    enum class Mode { Title, Play, Early, Hit, Chime, Leave, Fail, Pause };

    void begin();
    void turn(int dir);
    void strike();
    void leave();
    void fail(const char* why);
    void botAct();
    void human(const gs::Pad& pad);
    bool audit() const;
    int clockSec() const;
    bool onHour() const;
    bool pastHour() const;
    void wallFace(int& h, int& m, int& s) const;

    void draw();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, int pal, bool hflip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void place(int i, float& x, float& y) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    const char* reason_ = "";
    const char* face_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool chimed_ = false;
    int pad_ = 0;
    int sticks_ = kSticks;
    int blows_ = 0;
    int playFrames_ = 0;
    int rep_ = 0;
    int hold_ = 0;
    int anim_ = 0;
    float walk_ = 48.f;
};

}  // namespace drumchime
