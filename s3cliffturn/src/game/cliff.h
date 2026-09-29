#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace cliff {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CLIFFTURN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const std::string& summary() const { return summary_; }

private:
    enum class Mode { Title, Race, Tip, Win, Lose };

    void beginRace();
    void physics(float dt, float steer, float thr, float brk);
    void draw();
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void blip(float freq, float vol);
    float curveAt(float s) const;
    int cliffSide(float s) const;
    void project(float z, float lateral, float& sx, float& sy, float& ppm) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    std::string summary_;

    float t_ = 0;
    float s_ = 0;
    float x_ = 0.6f;
    float vx_ = 0;
    float speed_ = 0;
    float lean_ = 0;
    float clock_ = 0;
    float hold_ = 0;
    int turns_ = 0;
    int taken_[3] = {};
    int side_ = -1;
    float shake_ = 0;
    float xoff_[224] = {};
    float ppm_[224] = {};
    int horizon_ = 86;
};

}  // namespace cliff
