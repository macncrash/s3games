// S3 SALLY DAWN — keep the flares lit on the sally until morning.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace sally {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SALLY DAWN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }

private:
    enum class Mode { Title, Watch, Pause, Victory, Fail };

    void beginWatch();
    void update(float dt);
    void draw();
    void sky();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void stoke();
    int nearest() const;
    void blip(float freq);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    float px_ = 160;
    float face_ = 1;
    float fuels_[4] = {};
    float cds_ = 0;
    float left_ = 0;
    float clock_ = 0;
    float gust_ = 0;
    float gustT_ = 4;
    float flicker_ = 0;
    float titleT_ = 0;
    int fed_ = -1;
    uint32_t rng_ = 0x5A11u;
};

}  // namespace sally
