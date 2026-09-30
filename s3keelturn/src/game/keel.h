// S3 KEEL TURN — take the keel through three harbour turns without tipping.
// The running clock is the other crew.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace keel {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KEEL TURN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int turns() const { return turns_; }
    float raceTime() const { return raceT_; }
    float crewTime() const { return crewLimit_; }

private:
    enum class Mode { Title, Race, Tipped, Lost, Won };

    void begin();
    void update();
    void draw();
    void water();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0);
    bool project(float wx, float wy, float wz, float& sx, float& sy, float& sh, int& fog) const;
    void tip();
    void finish(bool win);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool tipped_ = false;
    bool got_[3] = {};
    int turns_ = 0;
    int heelFrame_ = 2;
    float t_ = 0;
    float raceT_ = 0;
    float hold_ = 0;
    float x_ = 0, z_ = 0, vx_ = 0;
    float sheet_ = 0.4f;
    float heel_ = 0;
    float steer_ = 0;
    float bob_ = 0;
    float crewLimit_ = 62.f;
};

}  // namespace keel
