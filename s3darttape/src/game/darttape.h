// S3 DARTTAPE — the drawer has to match the tape.
// A firm dart in D15, D12 or D18 drops that slip in the till.
// The treble with the same count stays out.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace darttape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DARTTAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const { return held_[0] && held_[1] && held_[2]; }
    bool held(int i) const { return i >= 0 && i < 3 && held_[i]; }
    bool rules() const { return rules_; }
    int darts() const { return darts_; }
    int traps() const { return traps_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* reason() const { return reason_; }
    const char* modeName() const;
    int phase() const;
    bool flying() const;

private:
    enum class Mode { Title, Aim, Flight, Pocket, Judge, Leave, Lose, Pause, Over };
    enum class Fate { True, Short, Hot };

    struct Pin {
        float x = 0, y = 0;
        bool on = false;
        bool board = false;
    };

    bool audit();
    void toTitle();
    void newGame();
    void beginAim();
    int nextOpen() const;
    void launch();
    void stick();
    void beginLeave();
    void beginLose();
    void afterPocket();
    void afterJudge();
    void moveAim(float mx, float my);
    void botAim();
    void humanAim(const gs::Pad& p, bool fire);
    void remember(float x, float y, bool board);
    bool sweet() const;

    void blip(int ch, float freq, float vol, float hold);
    void tickAudio(float dt);
    void backdrop();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool hflip = false, bool shadow = false);
    void sprM(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void stamp(const gs::Image& img, float x, float y, int pal, float jx = 0);
    void dartAt(float x, float y, float h);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode heldMode_ = Mode::Aim;
    Fate fate_ = Fate::True;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool held_[3] = {};
    bool wasSweet_ = false;
    int darts_ = 0;
    int traps_ = 0;
    int pinN_ = 0;
    int shake_ = 0;
    int flightT_ = 0;
    int flySlip_ = -1;
    char label_[3][8] = {};
    char reason_[32] = {};
    char last_[12] = {};
    float aimX_ = kCx;
    float aimY_ = kCy;
    float landX_ = kCx;
    float landY_ = kCy;
    float fromX_ = kHandX;
    float fromY_ = kHandY;
    float targetX_ = kCx;
    float targetY_ = kCy;
    float meter_ = 0;
    float meterDir_ = 1;
    float clock_ = 0;
    float pocketT_ = 0;
    float judgeT_ = 0;
    float leaveT_ = 0;
    float toneT_ = 0;
    float tickT_ = 0;
    float slipFromX_ = 0;
    float slipFromY_ = 0;
    Pin pin_[8];
};

}  // namespace darttape
