// S3 STRIKER — three swings at a carnival high striker. Ring the bell.
#pragma once
#include <vector>

#include "art.h"
#include "console/system.h"

namespace striker {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 STRIKER"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int swing() const { return winSwing_; }
    int best() const { return bestMark_; }
    // 0 title, 1 ready, 2 in the air, 3 bell, 4 result
    int marker() const;

private:
    enum class Mode { Title, Ready, Strike, Rise, Fall, Between, Bell, Win, Lose };

    struct Bit {
        float x, y, vx, vy, life;
        int kind;
    };

    void resetAttempt();
    void launch(float power);
    void ring();
    void update(float dt);
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void anchor(const gs::Mipped& m, float ax, float ay, float lx, float ly, float destH, int pal);
    void meterBar(int row);
    void blip(bool high);
    void thunk();
    void chime();
    void whistle(float h);
    float meter() const;
    int pose() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    int swings_ = 3;
    int used_ = 0;
    int winSwing_ = 0;
    int bestMark_ = 0;
    int strikeFrom_ = 0;
    float t_ = 0;
    float phase_ = 0;
    float power_ = 0;
    float apex_ = 0;
    float puck_ = 0;
    float puckV_ = 0;
    float modeT_ = 0;
    float bellT_ = 0;
    float flashT_ = 0;
    float shake_ = 0;
    float noteT_ = 0;
    float beepT_ = 0;
    int noteI_ = 0;
    int fanStep_ = -1;
    float fanT_ = 0;
    bool rising_ = false;
    uint32_t rng_ = 0x5C1A7u;
    std::vector<Bit> bits_;
};

}  // namespace striker
