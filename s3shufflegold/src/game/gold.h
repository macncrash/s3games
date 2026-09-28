// S3 SHUFFLE GOLD — one end. A gold disk in the 3 counts double.
// A cream disk scores the painted number and does not buy the double.
#pragma once
#include <string>
#include <vector>

#include "console/gfx.h"
#include "console/system.h"

namespace shufflegold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SHUFFLE GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int goldPts() const { return goldPts_; }
    int creamPts() const { return creamPts_; }
    int goldZone() const { return goldZone_; }
    int creamZone() const { return creamZone_; }

private:
    enum class Mode { Title, Aim, Roll, Between, Win, Lose };
    enum class Kind { Gold, Cream };

    struct Disk {
        float x = 0, y = 0;
        Kind kind = Kind::Gold;
        bool live = true;
        int zone = 0;
    };

    void buildArt();
    void layTable();
    void begin();
    void launch(float x, float targetY);
    void stepDisk();
    void onRest();
    void finishEnd();
    void backdrop();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);

    gs::System* sys_ = nullptr;
    Mode mode_ = Mode::Title;
    Kind hand_ = Kind::Gold;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool charging_ = false;
    bool hasDisk_ = false;
    bool live_ = true;
    bool rest_ = true;
    int goldPts_ = 0;
    int creamPts_ = 0;
    int goldZone_ = 0;
    int creamZone_ = 0;
    int frames_ = 0;
    int hold_ = 0;
    int sayPal_ = 0;
    float aimX_ = 160.f;
    float targetY_ = 52.f;
    float meter_ = 0.35f;
    float meterDir_ = 1.f;
    float x_ = 160.f, y_ = 172.f, vx_ = 0.f, vy_ = 0.f;
    std::string say_;
    std::vector<Disk> settled_;

    gs::Mipped disk_;
    gs::Mipped shadow_;
    gs::Mipped chev_;
    int font_[96] = {};
    int wax_ = 1, line_ = 1, z3_ = 1, z2_ = 1, z1_ = 1, railH_ = 1, railV_ = 1, wood_ = 1, gutter_ = 1;
};

}  // namespace shufflegold
