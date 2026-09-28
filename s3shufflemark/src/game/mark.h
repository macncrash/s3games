// S3 SHUFFLEMARK — slide the disk until it stops inside the painted mark.
#pragma once
#include <string>

#include "console/gfx.h"
#include "console/system.h"

namespace shufflemark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SHUFFLEMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }
    bool onMark() const { return onMark_; }
    int throws() const { return throws_; }

private:
    enum class Mode { Title, Aim, Roll, Miss, Win };

    void buildArt();
    void layTable();
    void begin();
    void launch(float x, float targetY);
    void stepDisk();
    void onRest();
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
    bool finished_ = false;
    bool onMark_ = false;
    bool charging_ = false;
    bool hasDisk_ = false;
    int throws_ = 0;
    int frames_ = 0;
    int hold_ = 0;
    int sayPal_ = 0;
    float aimX_ = 160.f;
    float targetY_ = 56.f;
    float meter_ = 0.35f;
    float meterDir_ = 1.f;
    float x_ = 160.f, y_ = 168.f, vx_ = 0.f, vy_ = 0.f;
    bool live_ = true;
    bool rest_ = true;
    std::string say_;

    gs::Mipped disk_;
    gs::Mipped shadow_;
    gs::Mipped chev_;
    int font_[96] = {};
    int wax_ = 1, line_ = 1, mark_ = 1, railH_ = 1, railV_ = 1, wood_ = 1, gutter_ = 1;
};

}  // namespace shufflemark
