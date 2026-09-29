// S3 FISHCHIME — a short fish. Done when the hour has to chime.
// Swim to the buoy bell. Strike only inside the hour, then leave.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace fishchime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FISHCHIME"; }
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
    int breaths() const { return breaths_; }
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Play, Early, Chime, Leave, Fail };

    void begin();
    void strike();
    void botAct();
    void human(const gs::Pad& pad);
    bool audit() const;
    int clockSec() const;
    int framesUntilHour() const;
    bool onHour() const;
    bool pastHour() const;
    void wallFace(int& h, int& m, int& s) const;
    bool nearBell() const;

    void draw();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, int pal, bool hflip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool pause_ = false;
    bool faceR_ = true;
    int breaths_ = kBreaths;
    int playFrames_ = 0;
    int hold_ = 0;
    int strikes_ = 0;
    int swing_ = 0;
    float fishX_ = 36.f;
    float bob_ = 0.f;
};

}  // namespace fishchime
