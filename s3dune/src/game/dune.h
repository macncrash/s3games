// S3 DUNE — one water stop. Don't boil the engine.
#pragma once
#include <cstdint>
#include <vector>

#include "art.h"
#include "console/system.h"

namespace dune {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DUNE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* report() const { return report_; }
    // 0 title, 1 countdown, 2 desert, 3 filling, 4 after the stop, 5 ended
    int marker() const;

private:
    enum class Mode { Title, Count, Run, Pause, Result };
    enum Kind : uint8_t { ROCK, SCRUB, TANK, PALM, BANNER_W, BANNER_C, TENT, POST };

    struct In {
        float steer = 0;
        float gas = 0;
        float brake = 0;
        bool reverse = false;
    };
    struct Thing {
        float z = 0, x = 0, h = 1;
        Kind kind = ROCK;
        bool hit = false;
    };
    struct Puff {
        float x = 0, y = 0, vx = 0, vy = 0, life = 0, s = 1;
        bool steam = false;
    };
    struct Proj {
        float x = 0, y = 0, z = 0, hw = 0, fog = 0;
        bool ok = false;
    };

    void poseTitle();
    void begin();
    void layout();
    void finish(bool win, int why);
    In human(const gs::Pad& pad) const;
    In botDrive() const;
    float autoSteer(bool reverse) const;
    void integrate(const In& in);
    bool inBox() const;
    float sandAmt() const;
    void buildShift();
    float shiftAt(float dz) const;
    Proj project(float wz, float wx) const;
    void blit(const gs::Mipped& m, float cx, float foot, float h, int pal, bool flip, int fog, bool shade = false);
    void ui(const gs::Image& im, float x, float y, int pal);
    void hudText(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void sky();
    void road();
    void world();
    void hud();
    void audio();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool watered_ = false;
    int why_ = 0;  // 1 camp, 2 boiled, 3 skipped the water
    int fillFrames_ = 0;
    int beep_ = -1;
    int fanStep_ = -1;
    float fanT_ = 0;
    float t_ = 0;
    float modeT_ = 0;
    float tRun_ = 0;
    float z_ = 0, x_ = 0, yaw_ = 0, speed_ = 0;
    float steer_ = 0, gas_ = 0, brake_ = 0;
    bool reverse_ = false;
    float heat_ = 18;
    float shake_ = 0;
    float flash_ = 0;
    float warnT_ = 0;
    float warnHold_ = 0;
    float toneT_ = 0;
    int hor_ = 104;
    float shiftX_[121] = {};
    float shiftH_[121] = {};
    std::vector<Thing> things_;
    std::vector<Puff> puffs_;
    char report_[192] = {};
};

}  // namespace dune
