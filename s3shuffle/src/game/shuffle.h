// S3 SHUFFLE — first to fifteen. The disk has to stop in the score.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace shuffle {

struct Disk {
    float x = 0, y = 0, vx = 0, vy = 0;
    int side = 0;
    bool live = true;
    bool rest = true;
    bool dead = false;
    bool told = false;
};

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SHUFFLE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int house() const { return house_; }
    int frames() const { return frames_; }
    std::string dump() const;

private:
    enum class Mode { Title, Aim, Roll, Score, Pause, Win, Lose };

    struct Cast {
        Disk disk;
        int you = 0;
        int house = 0;
    };

    void beginFrame();
    void layTable();
    void launch(int side, float x, float targetY);
    void botLaunch();
    void houseLaunch();
    void humanAim(const gs::Pad& pad);
    void updateGhost();
    Cast forecast(int side, float x, float targetY) const;
    void onRest();
    void applyScore();
    void noteStop(const Disk& d);
    void chime(int kind);
    void quiet();
    void sweetTick();
    void backdrop();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void puck(int side, float x, float y, float h);
    bool yourTurn() const { return next_ == 0; }

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool charging_ = false;
    bool ghostOn_ = false;
    bool sweet_ = false;
    int you_ = 0;
    int house_ = 0;
    int frames_ = 0;
    int next_ = 0;
    int thrown_[2] = {};
    int gainYou_ = 0;
    int gainHouse_ = 0;
    int sayPal_ = PAL_INK;
    float aimX_ = 160.f;
    float meter_ = 0.2f;
    float meterDir_ = 1.f;
    float t_ = 0;
    float aimT_ = 0;
    float rollT_ = 0;
    float toneT_ = 0;
    float sweetT_ = 0;
    float hitCd_ = 0;
    float ghostX_ = 0;
    float ghostY_ = 0;
    std::string say_;
    std::vector<Disk> disks_;
};

}  // namespace shuffle
