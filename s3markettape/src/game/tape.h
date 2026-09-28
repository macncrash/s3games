// S3 MARKETTAPE — the drawer has to match the tape.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace markettape {

constexpr int kTapeN = 3;
constexpr int kGoodN = 6;
constexpr int kTries = 3;

struct Good {
    const char* name;
    int pay;
    int line;  // tape line, or -1 if it only looks like the price
};

const Good* goods();
const char* tapeName(int i);
int tapePay(int i);
int tapeSum();

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MARKETTAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const;
    bool rules() const { return rules_; }
    const char* reason() const { return reason_; }
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    int drawerScore() const;
    int faults() const { return faults_; }
    bool heldLine(int line) const;

private:
    enum class Mode { Title, Play, Win, Lose };

    void begin();
    bool audit();
    void readInput();
    void driveBot();
    void toggle();
    void tryLeave();
    void logic();
    void audio();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    int drawerCount() const;
    bool inDrawer(int g) const;
    void blip(float freq);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool rules_ = false;
    int cursor_ = 0;
    int drawer_[3] = {-1, -1, -1};
    int faults_ = 0;
    int age_ = 0;
    int cool_ = 0;
    int shake_ = 0;
    int beepN_ = 0;
    float beepF_ = 0;
    char reason_[64] = {};
};

}  // namespace markettape
