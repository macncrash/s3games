// S3 BIKETURN — three corners. Lean with them or tip.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace biketurn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BIKETURN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int turns() const { return cleared_; }
    const char* result() const { return result_; }

private:
    enum class Mode { Title, Ride, Pause, Crash, Win };

    void resetRide();
    void ride(float dt);
    void finish(bool win);
    float steer() const;
    float needAt(float z) const;
    float bendAt(float z) const;
    int cornerAt(float z) const;
    void draw();
    void backdrop();
    void hud();
    void blit(const gs::Mipped& m, float x, float y, float h, int pal, bool flip = false);
    void image(const gs::Image& img, float x, float y, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Ride;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int cleared_ = 0;
    bool passed_[3] = {};
    char result_[160] = {};

    float t_ = 0;
    float modeT_ = 0;
    float z_ = 0;
    float lean_ = 0;
    float tip_ = 0;
    float speed_ = 16.f;
    int shake_ = 0;
};

}  // namespace biketurn
