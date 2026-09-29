// S3 CLIFFSLIP — berth the launch in the cliff slip before the tide turns.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace slip {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CLIFFSLIP"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float tideLeft() const { return tide_; }
    // 0 title, 1 on the water, 2 berth, 3 turned
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Win, Lose };

    void resetRun();
    void update(float dt);
    void physics(float dt, float steer, float thrust);
    void draw();
    void text(const std::string& s, float x, float y, int pal);
    void blit(const gs::Image& img, float x, float y, int pal, bool shadow = false);
    void botInput(float& steer, float& thrust);
    bool inSlip() const;
    static float wrap(float a);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    float t_ = 0;
    float tide_ = 0;
    float x_ = 0, y_ = 0, h_ = 0, vx_ = 0, vy_ = 0;
    float held_ = 0;
    float chime_ = 0;
    int gull_ = 0;
};

}  // namespace slip
