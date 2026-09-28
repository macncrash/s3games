// S3 LUGE PASS — one ice chute. Clear it before the storm clock dies.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace luge {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LUGE PASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int clockLeft() const { return int(clock_ + 0.5f); }
    const char* report() const { return report_; }
    // 0 title, 1 countdown, 2 the chute, 3 ended
    int marker() const;

private:
    enum class Mode { Title, Count, Run, Pause, Result };

    struct Spray {
        float x, y, vx, vy, life, s;
    };
    struct Rock {
        float z, x;
        bool hit;
    };
    struct Proj {
        float x = 0, y = 0, z = 0, hw = 0, fog = 0;
        bool ok = false;
    };

    void resetRun();
    void beginRun();
    void botDrive(float& steer, float& tuck, float& scrub) const;
    void humanDrive(const gs::Pad& pad, float& steer, float& tuck, float& scrub) const;
    void integrate(float steer, float tuck, float scrub);
    void endRun(int why);
    void buildShift();
    float shiftAt(float dz) const;
    Proj project(float wz, float wx) const;
    void blit(const gs::Mipped& m, float cx, float foot, float h, int pal, bool flip, int fog, bool shade = false);
    void ui(const gs::Image& im, float x, float y, int pal);
    void hudText(int col, int row, const char* s, int pal);
    void sky();
    void road();
    void world();
    void hud();
    void audio();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Run;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int why_ = 0;  // 1 cleared, 2 wall, 3 storm
    float clock_ = 0;
    float t_ = 0;
    float modeT_ = 0;
    float tRun_ = 0;
    float z_ = 0, x_ = 0, yaw_ = 0, speed_ = 0;
    float steer_ = 0, tuck_ = 0, scrub_ = 0;
    float shake_ = 0, flash_ = 0;
    float toneT_ = 0;
    int beep_ = -1;
    int fanStep_ = -1;
    float fanT_ = 0;
    int hor_ = 108;
    float shiftX_[121] = {};
    float shiftH_[121] = {};
    std::vector<Spray> spray_;
    std::vector<Rock> rocks_;
    char report_[180] = {};
};

}  // namespace luge
