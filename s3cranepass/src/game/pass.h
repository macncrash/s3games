// S3 CRANEPASS — lift the wreckage off the road before the other crew's storm clock.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace pass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CRANEPASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int cleared() const { return cleared_; }
    float clock() const { return clock_; }

private:
    enum class Mode { Title, Play, Win, Fail };
    enum class Job { ToPiece, LowerGrab, Raise, ToDump, LowerDrop };

    struct Piece {
        float x = 0;
        int kind = 0;
        bool gone = false;
    };

    void begin();
    void update(float dt);
    void botPlan(float& ax, float& hoist, bool& act);
    void draw();
    bool grab();
    void release();
    void hookAt(float& x, float& y) const;
    bool startPressed() const;
    bool actPressed() const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void blip(float freq);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Job job_ = Job::ToPiece;
    Piece pieces_[4]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool actLatch_ = false;
    int carrying_ = -1;
    int cleared_ = 0;
    int target_ = 0;
    float tx_ = 160.f;
    float vx_ = 0.f;
    float len_ = 28.f;
    float t_ = 0.f;
    float clock_ = 78.f;
    float melodyT_ = 0.f;
    int melody_ = -1;
};

}  // namespace pass
