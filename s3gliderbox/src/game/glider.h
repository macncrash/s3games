// S3 GLIDER BOX — the glider has one job: stop inside the box.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace gbox {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GLIDER BOX"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return float(time_); }
    const char* why() const { return why_; }
    // 0 title, 1 on the way, 2 at the box, 3 holding the stop, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Fly, Pause, Fail, Win };

    struct Puff {
        double x = 0, life = 0;
    };

    void showTitle();
    void startRun();
    void pilot(double& nose, double& spoil) const;
    void physics(double nose, double spoil);
    void win();
    void fail(const char* why, const char* banner);
    void blip(float freq);
    void draw(double x, double h, double att, double vy, double spd, bool craft);
    void sky();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip = false, int fog = 0);
    void sprAnchor(const gs::Mipped& m, float ax, float ay, float sx, float sy, float destH, int pal);
    void sprBox(const gs::Mipped& m, float cx, float top, float w, float h, int pal);
    bool hullInside() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool ground_ = false;
    bool touched_ = false;
    const char* why_ = "";
    const char* banner_ = "";
    int puffN_ = 0;
    int chime_ = -1;
    double time_ = 0;
    double x_ = 0, h_ = 0, v_ = 0, vy_ = 0;
    double att_ = 0;
    double nose_ = 0, spoil_ = 0;
    double hold_ = 0;
    double camX_ = 0, camH_ = 0, camS_ = 2;
    float chimeT_ = 0;
    float beep_ = 0;
    Puff puffs_[8]{};
};

}  // namespace gbox
