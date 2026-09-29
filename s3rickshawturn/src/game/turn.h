// S3 RICKSHAW TURN — make the three turns without tipping.
#pragma once
#include "art.h"
#include "console/system.h"

namespace rickshawturn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RICKSHAW TURN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_ ? why_ : ""; }
    float seconds() const { return clock_; }
    int turns() const { return turns_; }
    float lean() const { return lean_; }
    // 0 title, 1 the straight, 2 inside a turn, 3 the last turn, 4 finished
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
    void update(float dt);
    void finish(bool good, const char* why);
    void audio();
    void draw();
    void street(float view);
    void furniture(float along);
    void rickshawAt();
    void text(int col, int row, const char* s, int pal);
    void textC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0);
    Proj project(float wx, float ahead) const;
    float curveAt(float s) const;
    float bendAt(float s) const;
    float halfAt(float s) const;
    float needAt(float s, float speed) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    int turns_ = 0;
    int passed_ = 0;
    float t_ = 0, clock_ = 0, dist_ = 0, x_ = 0, vx_ = 0;
    float helm_ = 0, lean_ = 0, speed_ = 0, scenery_ = 0, slip_ = 0, bell_ = 0;
};

}  // namespace rickshawturn
