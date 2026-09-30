// S3 CULVERTDAWN — the culvert is yours. Keep the flares lit until dawn.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace culvertdawn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CULVERTDAWN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }

private:
    enum class Mode { Title, Play, Out, Dawn };

    void begin();
    void update(float dt);
    void botAct(bool& left, bool& right, bool& light);
    void draw();
    void spr(const gs::Image& img, float x, float y, float w, float h, int pal, int fog = 0, bool flip = false);
    void hud(int col, int row, const std::string& s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    float t_ = 0;
    float px_ = 36;
    float face_ = 1;
    float step_ = 0;
    float cool_ = 0;
    float strike_ = 0;
    int beat_ = 0;
    int dead_ = -1;
    float flare_[4] = {1, 1, 1, 1};
    static constexpr float kX[4] = {40.f, 114.f, 188.f, 268.f};
    static constexpr float kNight = 36.f;
};

}  // namespace culvertdawn
