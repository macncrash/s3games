// S3 SHELVE — one cart. A book in the wrong row comes back.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace shelve {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SHELVE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int shelved() const { return shelved_; }
    int returned() const { return returns_; }
    // 0 title, 1 aligning the cart, 2 the book is going out, 3 coming back, 4 clear, 5 the cart goes back
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Win, Lose };
    enum class Anim { None, Out, Hold, Back };

    static constexpr int kRows = 4;
    static constexpr int kBooks = 8;
    static constexpr int kMaxBack = 3;

    void toTitle();
    void begin();
    void enterWin();
    void enterLose();
    void readHuman();
    void botAct();
    void moveCursor(int dir);
    void shelve();
    void popFront();
    void approach();
    void stepAnim();
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
    void chime(int row);
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
    bool parked_ = true;
    int cursor_ = 0;
    int cart_[kBooks] = {};
    int cartN_ = 0;
    int on_[kRows] = {};
    int shelved_ = 0;
    int returns_ = 0;
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
    uint32_t rng_ = 1;
};

}  // namespace shelve
