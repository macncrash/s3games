// S3 BELLMARK — one hanging bell, one mark on the lip.
// Pull the rope at the top of the swing. Four clean strikes finish the mark.
// That finished mark ends the cartridge. A mistimed pull does not.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace bellmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BELLMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool finished() const { return finished_; }
    bool struck() const { return struck_; }
    int rings() const { return good_; }
    int pulls() const { return pulls_; }

private:
    enum class Mode { Title, Play, Show, Lose };

    void begin();
    void pull();
    void finishMark();
    void fail();
    void botPlay();
    bool inWindow() const;
    float swing() const;

    void ding();
    void thud();
    void hush();

    void draw();
    void backdrop();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool hflip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool finished_ = false;
    bool struck_ = false;
    bool botHeld_ = false;
    int good_ = 0;
    int pulls_ = 0;
    int misses_ = 0;
    float phase_ = 0;
    float modeT_ = 0;
    float flash_ = 0;
    float toneT_ = 0;
    float sway_ = 0;
};

}  // namespace bellmark
