// S3 MARKET SEVEN — one stall against the rival stall.
// Cream banks 1, loaf banks 2, gold banks 3, and only exact change banks it.
// First tally to reach 7 wins. A 6 is still short, and the match goes on.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace marketseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MARKET SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const;
    bool shortSix() const { return shortSix_; }
    int you() const { return you_; }
    int them() const { return them_; }
    const char* phase() const;

private:
    enum class Mode { Title, Pick, Till, Rival, Win, Lose, Pause };

    struct Sale {
        bool yours = false;
        int pts = 0;
    };
    struct Act {
        int dx = 0;
        bool drop = false;
        bool back = false;
        bool hand = false;
        bool start = false;
    };

    void toTitle();
    void resetMatch();
    void readInput();
    void driveBot();
    void logic();
    void audio();
    void bank(int pts);
    void miss();
    void beginRival();
    void finishRival();
    void winDay();
    void loseDay(const char* why);
    int rivalWant() const;
    int dueOf() const;
    void blip(float freq, int frames);

    void hud(int col, int row, const char* s);
    void hudAt(float cx, int row, const char* s);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void sprI(const gs::Image& img, float cx, float cy, float h, int pal);
    void solid(float x, float y, float w, float h, int pal);
    void shadeAt(float cx, float cy, float w);
    void digits(int n, float cx, float cy, int pal);
    void pips(int n, float x, float y, int pal);
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Pick;
    Act act_{};
    Sale log_[24]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool shortSix_ = false;
    int age_ = 0;
    int you_ = 0;
    int them_ = 0;
    int faults_ = 0;
    int good_ = 2;
    int coin_ = 0;
    int dish_ = 0;
    int stack_[8] = {};
    int stackN_ = 0;
    int logN_ = 0;
    int wait_ = 0;
    int rivalT_ = 0;
    int rivalPts_ = 0;
    int flashT_ = 0;
    int flashK_ = 0;
    int beepN_ = 0;
    float beepF_ = 0;
    float beepV_ = 0;
    const char* why_ = "";
};

}  // namespace marketseven
