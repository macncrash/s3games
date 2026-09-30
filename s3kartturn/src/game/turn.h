// S3 KARTTURN — three corners in the kart. Match the lean or tip.
// The leg ends at the banner. Missing it fails the run.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace kartturn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KARTTURN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int turns() const { return cleared_; }
    const char* result() const { return result_; }

private:
    enum class Mode { Title, Ride, Pause, Fail, Win };

    void resetRide();
    void ride(float dt);
    void finish(bool win, const char* why);
    float steerIn() const;
    float gasIn() const;
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
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int cleared_ = 0;
    bool passed_[3] = {};
    char result_[160] = {};

    float t_ = 0;
    float modeT_ = 0;
    float clock_ = 0;
    float z_ = 0;
    float lean_ = 0;
    float lat_ = 0;
    float tip_ = 0;
    float speed_ = 12.f;
    int shake_ = 0;
};

}  // namespace kartturn
