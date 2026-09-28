// S3 CRANEBOX — stop the crane inside the box. Missing the end fails the leg.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace cranebox {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CRANEBOX"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Run, Fail, Win };

    void begin();
    void update(float dt);
    void botPlan(float& gas, float& brake) const;
    bool inside() const;
    void win();
    void fail(const char* why);
    void draw();
    void sky();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void blip(float freq);
    void chime(float dt);
    float cam() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int melody_ = -1;
    float x_ = 0, v_ = 0;
    float time_ = 0, hold_ = 0, t_ = 0, melodyT_ = 0;
    const char* why_ = "the leg ran out";
};

}  // namespace cranebox
