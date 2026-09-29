// S3 STRIKER — the carnival tower. Ring the bell before the third try dies.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace striker {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 STRIKER"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int tries() const { return tries_; }

private:
    enum class Mode { Title, Ready, Meter, Flight, Mark, Win, Lose };

    void begin();
    void swing();
    void tick(float dt);
    void draw();
    void spr(const gs::Image& img, float x, float y, int w, int h, int pal);
    void label(const gs::Image& img, float x, float y);
    void chime();
    void thud();
    void tickAudio();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int tries_ = 0;
    int age_ = 0;
    int phase_ = 0;
    float meter_ = 0;
    bool rising_ = true;
    float power_ = 0;
    float puck_ = 0;  // 0 at the base, 1 at the bell
    float puckV_ = 0;
    bool ding_ = false;
    int bellFlash_ = 0;
};

}  // namespace striker
