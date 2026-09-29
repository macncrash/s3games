// S3 SHELVE CHIME — one cart. A book in the wrong row comes back.
// The hour has to chime: a matching shelf while the clock is on twelve.
// Early matches stay put and do not buy the hour. Three returns, or a missed
// twelve, and the cart goes back.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace shelvechime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SHELVE CHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool onTheHour() const { return onHour(); }
    int hour() const;
    int minute() const;
    int second() const;
    int shelved() const { return shelved_; }
    int returned() const { return returns_; }
    const char* reason() const { return reason_; }

    static constexpr int kGraceSec = 6;

private:
    enum class Mode { Title, Play, Pause, Win, Lose };
    enum class Anim { None, Out, Hold, Back };

    static constexpr int kRows = 4;
    static constexpr int kBooks = 4;
    static constexpr int kMaxBack = 3;
    static constexpr int kTick = 12;  // frames per clock second
    static constexpr int kStartSec = 11 * 3600 + 59 * 60 + 48;

    void toTitle();
    void begin();
    void enterWin();
    void enterLose(const char* why);
    void readHuman();
    void botAct();
    void moveCursor(int dir);
    void shelve();
    void popFront();
    void approach();
    void stepAnim();
    void clockWatch();
    void ticks();
    void draw();
    void wall();
    void drawAisle(bool live);
    void drawClock(float cx, float cy, float swing);
    void drawTitle();
    void flightXY(float& x, float& y) const;
    void handXY(float& x, float& y) const;
    void slotXY(int row, int slot, float& x, float& y) const;
    float rowY(int i) const { return ROW_Y0 + float(i) * ROW_DY; }
    int clockSec() const;
    bool onHour() const;
    bool pastHour() const;
    void face(int& h, int& m, int& s) const;
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void blip(float freq);
    void note(int row);
    void hourStrike();
    void reject();
    void push();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Anim anim_ = Anim::None;
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool parked_ = true;
    bool flightOnHour_ = false;
    int cursor_ = 0;
    int cart_[kBooks] = {};
    int cartN_ = 0;
    int on_[kRows] = {};
    int shelved_ = 0;
    int returns_ = 0;
    int playFrames_ = 0;
    int hold_ = 0;
    int msg_ = 0;
    int badRow_ = -1;
    int heldUp_ = 0;
    int heldDn_ = 0;
    int tone_ = 0;
    int push_ = 0;
    int fan_ = -1;
    int strike_ = 0;
    int endHold_ = 0;
    int flightBook_ = 0;
    int flightRow_ = 0;
    int flightSlot_ = 0;
    float flightT_ = 0;
    float flightX0_ = 0;
    float flightY0_ = 0;
    float cartY_ = ROW_Y0;
};

}  // namespace shelvechime
