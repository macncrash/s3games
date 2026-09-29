// S3 DRAWERCHIME — Drawer: the hour has to chime.
// Seat the hour, minute, and second plates in the chest.
// Pull the chime drawer only while the wall is on that hour.
// An early or false pull jams a try. Leaving after the chime is the win.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace drawerchime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DRAWERCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool chimed() const { return chimed_; }
    bool rules() const { return rules_; }
    bool onTheHour() const { return onHour(); }
    bool platesTrue() const { return plateHour() == kTarget && plateMinute() == 0 && plateSecond() == 0; }
    int hour() const;
    int minute() const;
    int second() const;
    int plateHour() const { return hourP_; }
    int plateMinute() const { return minP_; }
    int plateSecond() const { return secP_; }
    int tries() const { return tries_; }
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Play, Jam, Chime, Leave, Fail };

    void begin();
    void nudge(int dir);
    void pull();
    void botAct();
    void human(const gs::Pad& pad);
    bool audit() const;
    int clockSec() const;
    bool onHour() const;
    bool pastHour() const;
    void wallFace(int& h, int& m, int& s) const;

    void draw();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool chimed_ = false;
    bool rules_ = false;
    bool pause_ = false;
    int hourP_ = 9;
    int minP_ = 14;
    int secP_ = 33;
    int sel_ = 0;
    int tries_ = kTries;
    int playFrames_ = 0;
    int rep_ = 0;
    int hold_ = 0;
    int strikes_ = 0;
    int swing_ = 0;
    int slide_[kDrawers] = {};
    float clerkX_ = 28.f;
};

}  // namespace drawerchime
