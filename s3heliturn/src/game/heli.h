// S3 HELITURN — three banked turns in the heli. Tip, or miss the end, and the leg fails.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace heliturn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HELITURN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int turns() const { return cleared_; }
    const char* result() const { return result_; }

private:
    enum class Mode { Title, Fly, Pause, Fail, Win };

    void resetLeg();
    void fly(float dt);
    void finish(bool win, const char* why);
    float steer() const;
    float climb() const;
    float torqueAt(float x) const;
    int turnAt(float x) const;
    void draw();
    void sky();
    void hud();
    void blit(const gs::Mipped& m, float x, float y, float h, int pal, bool flip = false);
    void image(const gs::Image& img, float x, float y, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Fly;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int cleared_ = 0;
    bool passed_[3] = {};
    bool gated_ = false;
    char result_[160] = {};

    float t_ = 0;
    float modeT_ = 0;
    float x_ = 0;
    float y_ = 0;
    float vy_ = 0;
    float roll_ = 0;
    float rollV_ = 0;
    float clock_ = 0;
    int shake_ = 0;
};

}  // namespace heliturn
