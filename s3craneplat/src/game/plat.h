// S3 CRANEPLAT — take the crane and stop level with the platform.
// The clock is the other crew.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace craneplat {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CRANEPLAT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Shift, Win, Fail };

    void begin();
    void update(float dt);
    void draw();
    void botPlan(float& ax, float& hoist, bool& act) const;
    void hookAt(float& x, float& y) const;
    void crateAt(float& x, float& top, float& bot) const;
    bool overPlatform(float x) const;
    bool isLevel() const;
    bool startPressed() const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    float tx_ = 78.f, vx_ = 0.f;
    float len_ = 52.f;
    float th_ = 0.18f, om_ = 0.f;
    float t_ = 0.f;
    float crew_ = 24.f;
    float hold_ = 0.f;
    const char* why_ = "the other crew took the platform";
    const char* hint_ = "STOP LEVEL WITH THE PLATFORM";
};

}  // namespace craneplat
