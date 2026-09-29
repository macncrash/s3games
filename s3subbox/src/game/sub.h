// S3 SUB BOX — in the sub, you stop inside the box. Missing the end fails the leg.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace subbox {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SUB BOX"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return raceT_; }

private:
    enum class Mode { Title, Dive, Win, Lose };

    void beginLeg();
    void update(float dt);
    void draw();
    void pilot(float& thrust, float& ballast);
    void controls(float& thrust, float& ballast);
    bool hullInside() const;
    float noseX() const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void place(const gs::Mipped& m, float wx, float wy, float h, int pal, bool hflip = false);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int leg_ = 0;
    float t_ = 0;
    float raceT_ = 0;
    float clock_ = 0;
    float x_ = 0, y_ = 0, vx_ = 0, vy_ = 0;
    float camX_ = 0, camY_ = 0;
    float hold_ = 0;
    float still_ = 0;
    float tone_ = 0;
    float banner_ = 0;
    std::string why_;
};

}  // namespace subbox
