// S3 PLOWSLIP — take the plow and berth in the slip before the tide turns.
// The clock is the other crew.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace plow {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PLOWSLIP"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    float tideLeft() const { return tide_; }

private:
    enum class Mode { Title, Run, Win, Lose };

    void resetRun();
    void update(float dt);
    void draw();
    void stick(float& steer, bool& thrust, bool& brake);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void text(const std::string& s, float x, float y, int pal, int align = 0);
    void tone(float freq, float vol);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int score_ = 0;
    float t_ = 0;
    float tide_ = 0;
    float x_ = 160, y_ = 190, yaw_ = 0, speed_ = 0;
    float hold_ = 0;
    float banner_ = 0;
    float scrape_ = 0;
    bool berthed_ = false;
    int scrapes_ = 0;
};

}  // namespace plow
