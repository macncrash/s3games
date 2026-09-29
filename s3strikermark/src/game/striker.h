// S3 STRIKERMARK — one carnival tower.
// Painted marks climb the slot. Only the gold mark rings the bell.
// That finished mark ends the cartridge. A lower mark is still open.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace strikermark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 STRIKERMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool finished() const { return finished_; }
    int swings() const { return used_; }
    int mark() const { return landed_; }
    bool gold() const { return landed_ == kGold && finished_; }

private:
    enum class Mode { Title, Ready, Strike, Rise, Show, Win, Lose };

    void begin();
    void launch(float power);
    void settle();
    void finishMark();
    void fail();
    void botPlay();
    float meter() const;
    int pose() const;

    void blip(bool high);
    void thunk();
    void chime();
    void whistle(float h);

    void draw();
    void backdrop();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void img(const gs::Image& im, float cx, float cy, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void meterBar(int row);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool finished_ = false;
    int left_ = 3;
    int used_ = 0;
    int landed_ = -1;
    int best_ = -1;
    float phase_ = 0;
    float power_ = 0;
    float apex_ = 0;
    float puck_ = 0;
    float puckV_ = 0;
    float modeT_ = 0;
    float bellT_ = 0;
    float flash_ = 0;
};

}  // namespace strikermark
