// S3 CLOCKCHIME — play clock until the hour has to chime. Leave when that is true.
// Set the three hands on the posted hour, then haul. A true haul before twelve
// is early. Only a true haul inside the hour, followed by leaving, is the chime.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace clockchime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CLOCKCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool onTheHour() const { return onHour(); }
    bool handsTrue() const { return hour_ == kTarget && minute_ == 0 && second_ == 0; }
    int hour() const;
    int minute() const;
    int second() const;
    int towerHour() const { return hour_; }
    int ropes() const { return ropes_; }
    int posted() const { return kTarget; }
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Play, Early, Chime, Leave, Fail };

    void begin();
    void turn(int dir);
    void haul();
    void botAct();
    void human(const gs::Pad& pad);
    bool audit() const;
    int clockSec() const;
    int framesUntilHour() const;
    bool onHour() const;
    bool pastHour() const;
    void wallFace(int& h, int& m, int& s) const;
    int hourStep() const;

    void draw();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, int pal);
    void axle(const gs::Image& img, int pal);
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
    int hour_ = 10;
    int minute_ = 47;
    int second_ = 22;
    int grip_ = 0;
    int ropes_ = kRopes;
    int playFrames_ = 0;
    int rep_ = 0;
    int hold_ = 0;
    int strikes_ = 0;
    int swing_ = 0;
    int ropeY_ = 0;
    float keepX_ = 196.f;
};

}  // namespace clockchime
