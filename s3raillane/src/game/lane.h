// S3 RAILLANE — one cab, one lane, three legs.
// Stay inside the lane the whole leg. Missing the end pocket fails the leg.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace lane {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RAILLANE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int legsHeld() const { return held_; }
    const char* endNote() const { return note_; }
    // 0 title, 1 rolling, 2 leg clear, 3 ended
    int marker() const;

private:
    enum class Mode { Title, Roll, Banner, Pause, Won, Lost };

    void startRun();
    void beginLeg();
    void updateTitle(float dt);
    void updateRoll(float dt);
    void updateBanner(float dt);
    void updateEnd(float dt);
    void fail(const char* why);
    void holdLeg();
    void botSteer(float& steer, float& thr) const;
    float curveAt(int leg, float dist) const;
    void audio(float dt);

    void draw();
    void skyAndRoad();
    void hudText(int col, int row, const char* s, int pal);
    void hudCenter(int row, const char* s, int pal);
    void blit(const gs::Image& img, float x, float y, int pal, int w = 0, int h = 0);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode heldMode_ = Mode::Roll;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool engineOn_ = false;
    int leg_ = 0;
    int held_ = 0;
    float x_ = 0;
    float speed_ = 0;
    float dist_ = 0;
    float legTime_ = 0;
    float out_ = 0;
    float banner_ = 0;
    float t_ = 0;
    float blip_ = 0;
    const char* note_ = "";
};

}  // namespace lane
