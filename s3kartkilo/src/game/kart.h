// S3 KART KILO — finish the kilometer without touching a wheel.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace kartkilo {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KART KILO"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    // 0 title, 1 away, 2 among the wheels, 3 the last stretch, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Disk {
        double x = 0, y = 0, r = 1;
        double amp = 0, rate = 0, phase = 0;
        int kind = 0;  // 0 parked kart, 1 loose wheel
    };

    struct Puff {
        double x = 0, y = 0, life = 0;
    };

    void showTitle();
    void startRun();
    void layCourse();
    double diskX(const Disk& d, double t) const;
    void pilot(double& steer, bool& gas, bool& brake) const;
    void physics(double steer, bool gas, bool brake);
    void win();
    void fail(const char* why);
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool danger_ = false;
    int meters_ = 0;
    double t_ = 0, time_ = 0;
    double x_ = 0, y_ = 0, speed_ = 0;
    float zoom_ = 13.f;
    float tone_ = 0;
    std::vector<Disk> disks_;
    Puff puffs_[8]{};
    int puffN_ = 0;
    char why_[72] = {};
};

}  // namespace kartkilo
