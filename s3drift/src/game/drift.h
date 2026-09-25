// S3 DRIFT — one walled pass. The slide is the score. The wall ends it.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace drift {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DRIFT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return int(score_ + 0.5f); }
    const char* report() const { return report_; }
    // 0 title, 1 countdown, 2 the pass, 3 ended
    int marker() const;

private:
    enum class Mode { Title, Count, Run, Pause, Result };

    struct Puff {
        float x, y, vx, vy, life, s;
    };
    struct Proj {
        float x = 0, y = 0, z = 0, hw = 0, fog = 0;
        bool ok = false;
    };

    void resetLine();
    void beginPass();
    void botDrive(float& steer, float& gas, float& brake) const;
    void humanDrive(const gs::Pad& pad, float& steer, float& gas, float& brake) const;
    void integrate(float steer, float gas, float brake);
    void endPass(bool walled);
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
    bool walled_ = false;
    float score_ = 0;
    float hold_ = 0;
    float chain_ = 1;
    float t_ = 0;
    float modeT_ = 0;
    float tRun_ = 0;
    float z_ = 0, x_ = 0, yaw_ = 0, beta_ = 0, speed_ = 0;
    float steer_ = 0, gas_ = 0, brake_ = 0;
    float shake_ = 0, flash_ = 0;
    float toneT_ = 0;
    int beep_ = -1;
    int fanStep_ = -1;
    float fanT_ = 0;
    int hor_ = 96;
    float shiftX_[101] = {};
    float shiftH_[101] = {};
    std::vector<Puff> puffs_;
    char report_[160] = {};
};

}  // namespace drift
