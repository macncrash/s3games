// S3 SHELVE SEVEN — a short shelve. First to seven.
// A book on the wrong row comes back and does not count.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace shelveseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SHELVE SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }
    int returned() const { return returns_; }

private:
    enum class Mode { Title, Play, Pause, Win, Lose };
    enum class Anim { None, Out, Hold, Back };

    static constexpr int kRows = 4;
    static constexpr int kGoal = 7;
    static constexpr int kQueue = 6;
    static constexpr int kThemEvery = 42;

    void toTitle();
    void begin();
    void enterWin();
    void enterLose();
    void readHuman();
    void botAct();
    void moveCursor(int dir);
    void shelve();
    void pushBook();
    void approach();
    void stepAnim();
    void rival();
    void draw();
    void wall();
    void aisle(bool live);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void panel(const gs::Mipped& m, float cx, float cy, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void blip(float freq);
    void chime(int row);
    void reject();
    float rowY(int i) const { return ROW_Y0 + float(i) * ROW_DY; }
    uint32_t rnd();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Anim anim_ = Anim::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int cursor_ = 0;
    int queue_[kQueue] = {};
    int qn_ = 0;
    int you_ = 0;
    int them_ = 0;
    int returns_ = 0;
    int themWait_ = 0;
    int hold_ = 0;
    int tone_ = 0;
    int msg_ = 0;
    int badRow_ = -1;
    int heldUp_ = 0;
    int heldDn_ = 0;
    int flightBook_ = 0;
    int flightRow_ = 0;
    float flightT_ = 0;
    float cartY_ = ROW_Y0;
    uint32_t rng_ = 1;
};

}  // namespace shelveseven
