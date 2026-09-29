// S3 SHELVEBELL — shelve the cart. A wrong row kills the try.
// The bell rings only when the cart is clear, and only before the third try dies.
// Leave once that is true.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace shelvebell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SHELVEBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const { return rules_; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }
    int shelved() const { return shelved_; }
    const char* phase() const;
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Play, Pause, Ring, Leave, Over };
    enum class Anim { None, Out, Hold, Back };

    static constexpr int kRows = 4;
    static constexpr int kBooks = 4;
    static constexpr int kMaxDead = 3;

    void toTitle();
    void begin();
    void readHuman();
    void botAct();
    void moveCursor(int dir);
    void shelve();
    void popFront();
    void approach();
    void stepAnim();
    void ring();
    void dieTry();
    void ticks();
    void draw();
    void wall();
    void drawAisle(bool live);
    void drawTitle();
    void flightXY(float& x, float& y) const;
    void handXY(float& x, float& y) const;
    void slotXY(int row, int slot, float& x, float& y) const;
    float rowY(int i) const { return ROW_Y0 + float(i) * ROW_DY; }
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void panel(const gs::Mipped& m, float cx, float cy, float w, float h, int pal);
    void blip(float freq);
    void note(int row);
    void reject();
    void push();
    uint32_t rnd();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Anim anim_ = Anim::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool rules_ = false;
    bool parked_ = true;
    int cursor_ = 0;
    int cart_[kBooks] = {};
    int cartN_ = 0;
    int on_[kRows] = {};
    int shelved_ = 0;
    int dead_ = 0;
    int tryNo_ = 0;
    int hold_ = 0;
    int msg_ = 0;
    int badRow_ = -1;
    int heldUp_ = 0;
    int heldDn_ = 0;
    int tone_ = 0;
    int push_ = 0;
    int fan_ = -1;
    int flightBook_ = 0;
    int flightRow_ = 0;
    int flightSlot_ = 0;
    float flightT_ = 0;
    float flightX0_ = 0;
    float flightY0_ = 0;
    float cartY_ = ROW_Y0;
    float bellPh_ = 0;
    float bellAmp_ = 0.12f;
    float ringT_ = 0;
    float leaveT_ = 0;
    const char* why_ = "";
    uint32_t rng_ = 1;
};

}  // namespace shelvebell
