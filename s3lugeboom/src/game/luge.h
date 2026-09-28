// S3 LUGE BOOM — take the luge, deliver the drive to the boom.
// The clock is the other crew.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace luge {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LUGE BOOM"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* result() const { return result_; }
    float traveled() const { return s_; }

private:
    enum class Mode { Title, Run, Pause, Victory, Fail };

    struct Prop {
        float s, x;
        int kind;  // 0 tree, 1 rock
    };

    void resetRun();
    void update(float dt);
    void draw();
    void cacheBend();
    float bendTo(float dist) const;
    float curvature(float s) const;
    bool project(float wx, float wz, float& sx, float& sy, float& scale, int& fog) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void steerOf(float& steer, bool& tuck, bool& brake) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    char result_[160] = {};
    float s_ = 0, x_ = 0, speed_ = 0;
    float clock_ = 0, elapsed_ = 0, t_ = 0;
    float shake_ = 0, scrapeCd_ = 0, lean_ = 0;
    int drive_ = 3;
    float rival0_ = 0;
    float bend_[64] = {};
    Prop props_[28] = {};
    int propN_ = 0;
    bool windOn_ = false;
};

}  // namespace luge
