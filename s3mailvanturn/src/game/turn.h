// S3 MAIL VAN TURN — three bends. Take them upright. Too much lean tips the van.
#pragma once
#include "art.h"
#include "console/system.h"

namespace mailvanturn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MAIL VAN TURN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_ ? why_ : ""; }
    float seconds() const { return clock_; }
    float lean() const { return lean_; }
    float lateral() const { return x_; }
    float along() const { return dist_; }
    int turns() const { return turns_; }
    // 0 title, 1 the straight, 2 in a bend, 3 two turns made, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Win, Fail };

    struct Proj {
        float x = 0, y = 0, ppm = 0;
        int fog = 0;
        bool ok = false;
    };

    void toTitle();
    void begin();
    void finish(bool good, const char* why);
    void update(float dt);
    void pilot(float& steer, float& throttle);
    void markTurns(float prev);
    const char* tipWhy() const;
    const char* missWhy() const;
    void audio();
    void draw();
    void street(float view);
    void sides(float along);
    void vanAt();
    void text(int col, int row, const char* s, int pal);
    void textC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0, bool shadow = false);
    Proj project(float wx, float ahead) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool made_[3] = {};
    const char* why_ = "";
    int turns_ = 0;
    float t_ = 0, clock_ = 0, dist_ = 0, x_ = 0, vx_ = 0, helm_ = 0, speed_ = 0, lean_ = 0, leanVel_ = 0;
    float idle_ = 0, scenery_ = 0;
};

}  // namespace mailvanturn
