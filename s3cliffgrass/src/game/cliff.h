// S3 CLIFF GRASS — the cliff has one job: land on the grass and come to a full stop.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace cliffgrass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CLIFF GRASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return raceT_; }

private:
    enum class Mode { Title, Run, Win, Lose };

    void begin();
    void update(float dt);
    void draw();
    void pilot(float& gas, float& brake, float& steer);
    void controls(float& gas, float& brake, float& steer);
    bool onGrass() const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    bool project(float lat, float z, float& sx, float& sy, float& sh) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    float t_ = 0;
    float raceT_ = 0;
    float clock_ = 0;
    float z_ = 0;
    float u_ = 0;
    float speed_ = 0;
    float hold_ = 0;
    float still_ = 0;
    float tone_ = 0;
    std::string why_;
};

}  // namespace cliffgrass
