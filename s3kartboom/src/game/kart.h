// S3 KARTBOOM — deliver the drive to the boom.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace kartboom {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KARTBOOM"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int drive() const { return drive_; }
    int score() const { return score_; }

private:
    enum class Mode { Title, Run, Win, Fail };

    struct Cone {
        float z;
        float x;
        bool hit;
    };

    void startRun();
    void physics(float steer, float gas, float brake);
    void judge();
    void bot(float& steer, float& gas, float& brake);
    void draw();
    void spr(const gs::Mipped& m, float cx, float foot, float h, int pal, bool flip = false, int fog = 0);
    void glyphText(const char* s, int x, int y, int scale, int pal);
    float center(float z) const;
    void paintRoad();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int drive_ = 100;
    int score_ = 0;
    float t_ = 0;
    float z_ = 0;
    float x_ = 0;
    float vx_ = 0;
    float speed_ = 0;
    float hold_ = 0;
    float shake_ = 0;
    float flash_ = 0;
    float lift_ = 0;
    int hits_ = 0;
    Cone cones_[10]{};
    int ncones_ = 0;
    char why_[48] = {};
};

}  // namespace kartboom
