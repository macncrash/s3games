// S3 KARTPLAT — stop the kart level with the platform.
// Past the end of the deck, the leg is missed.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace kart {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KARTPLAT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int leg() const { return leg_; }
    int lives() const { return lives_; }
    int made() const { return made_; }
    // 0 title, 1 driving, 2 leg made, 3 missed, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Run, Banner, Miss, Over, Victory };

    void beginLeg();
    void update(float dt, bool throttle, bool brake);
    void miss();
    void clearLeg();
    void draw();
    void sky();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    float sx(float wx, float par = 1.f) const;
    void bot(bool& throttle, bool& brake) const;
    void blip(float freq);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int leg_ = 0;
    int lives_ = 3;
    int made_ = 0;
    float x_ = 0, v_ = 0;
    float phase_ = 0;
    float settle_ = 0;
    float banner_ = 0;
    float t_ = 0;
    bool wasThrottle_ = false;
    uint64_t tick_ = 0;
};

}  // namespace kart
