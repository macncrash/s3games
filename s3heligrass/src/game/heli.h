// S3 HELIGRASS — land on the grass and come to a full stop before the other crew.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace heli {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HELIGRASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }

private:
    enum class Mode { Title, Fly, Win, Lose };

    void resetFlight();
    void update(float dt);
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void text(const std::string& s, float x, float y, float scale, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    float x_ = 0, y_ = 0, vx_ = 0, vy_ = 0;
    float hold_ = 0;
    float t_ = 0;
    float msgT_ = 0;
    float rivalX_ = 0, rivalY_ = 0;
    int rotor_ = 0;
    int loseKind_ = 0;  // 1 drink, 2 crew
};

}  // namespace heli
