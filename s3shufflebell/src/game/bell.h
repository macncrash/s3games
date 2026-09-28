// S3 SHUFFLEBELL — slide the disk until the bell rings.
// Three tries. A disk that stops in the bell rings it. Anything else dies.
// The third dead try ends the table. Leave only after the bell.
#pragma once
#include <string>

#include "console/gfx.h"
#include "console/system.h"

namespace shufflebell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SHUFFLEBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const { return rules_; }
    bool sliding() const { return mode_ == Mode::Roll && hasDisk_; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }

private:
    enum class Mode { Title, Aim, Roll, Dead, Ring, Leave, Over };

    void buildArt();
    void layTable();
    void begin();
    void launch(float x, float targetY, bool trueShot);
    void stepDisk();
    void onRest();
    void ring();
    void dieTry();
    bool inBell(float x, float y) const;
    void backdrop();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);

    gs::System* sys_ = nullptr;
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool rules_ = false;
    bool charging_ = false;
    bool hasDisk_ = false;
    bool trueShot_ = false;
    int dead_ = 0;
    int tryNo_ = 0;
    int frames_ = 0;
    int hold_ = 0;
    int sayPal_ = 0;
    float aimX_ = 160.f;
    float targetY_ = 52.f;
    float meter_ = 0.f;
    float meterDir_ = 1.f;
    float x_ = 160.f, y_ = 168.f, vx_ = 0.f, vy_ = 0.f;
    float bellPh_ = 0.f;
    float bellAmp_ = 0.2f;
    bool live_ = true;
    bool rest_ = true;
    std::string say_;

    gs::Mipped disk_;
    gs::Mipped shadow_;
    gs::Mipped chev_;
    gs::Mipped bell_;
    gs::Mipped clapper_;
    int font_[96] = {};
    int wax_ = 1, line_ = 1, bellTile_ = 1, railH_ = 1, railV_ = 1, wood_ = 1, gutter_ = 1;
};

}  // namespace shufflebell
