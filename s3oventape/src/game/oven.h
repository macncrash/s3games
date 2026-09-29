// S3 OVENTAPE — play the oven until the drawer matches the tape, then leave.
// The tape wants CRUST, GOLD and MILK. CHAR and SOOT pay nothing and spoil the till.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace oventape {

constexpr int kTapeN = 3;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 OVENTAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const { return held_[0] && held_[1] && held_[2]; }
    bool held(int i) const { return i >= 0 && i < kTapeN && held_[i]; }
    bool rules() const { return rules_; }
    int pulls() const { return pulls_; }
    int traps() const { return traps_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* reason() const { return reason_; }
    const char* modeName() const;

private:
    enum class Mode { Title, Bake, Leave, Lose };

    bool audit() const;
    void toTitle();
    void begin();
    void advance();
    void pull();
    void skip();
    void burn();
    void openDoor();
    void walkOut();
    void blip(float freq);
    void backdrop();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void word(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool held_[kTapeN] = {};
    int idx_ = 0;
    int pulls_ = 0;
    int traps_ = 0;
    float heat_ = 0;
    float clock_ = 0;
    float sayT_ = 0;
    float leaveT_ = 0;
    char say_[40] = {};
    char reason_[48] = {};
};

}  // namespace oventape
