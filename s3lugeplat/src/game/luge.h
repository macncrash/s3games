// S3 LUGE — one job: stop level with the platform.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace luge {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LUGE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    float miss() const { return miss_; }

private:
    enum class Mode { Title, Ride, Done };

    static constexpr float LIP = 980.f;
    static constexpr float FLAT = 700.f;
    static constexpr float DRAG = 0.992f;
    static constexpr float GRADE = 0.030f;
    static constexpr float BRAKE = 0.20f;
    static constexpr float TOL = 4.0f;

    void begin();
    void stepPhys(float& x, float& v, bool brake) const;
    float coastStop(float x, float v, int coast) const;
    bool wantBrake() const;
    void hud(int col, int row, const std::string& s);
    void hudC(int row, const std::string& s);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void draw();
    float cam() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool braking_ = false;
    int score_ = 0;
    float miss_ = 0;
    float x_ = 20.f;
    float v_ = 0.6f;
    float drop_ = 0;
    int beep_ = 0;
    int ticks_ = 0;
};

}  // namespace luge
