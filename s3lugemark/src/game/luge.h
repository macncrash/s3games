// S3 LUGE MARK — take the luge and set down on the mark.
// The clock beside the chute is the other crew. When it runs out, they own the mark.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace luge {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LUGE MARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return race_; }
    float left() const { return limit_ - race_ > 0 ? limit_ - race_ : 0; }
    // 0 title, 1 on the ice, 2 over the mark, 3 holding the set-down, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void showTitle();
    void startRun();
    void controls(float& steer, float& brake);
    void pilot(float& steer, float& brake);
    void physics(float steer, float brake);
    void judge();
    void win();
    void fail(const char* why);
    void blip(float freq);
    void chime();
    void audio();
    void draw();
    void drawRoad();
    void drawWorld();
    void drawHud();
    bool project(float wz, float wx, float& sx, float& sy, float& scale) const;
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false, bool hflip = false);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int phase_ = 0;
    int chimeStep_ = -1;
    float t_ = 0, race_ = 0;
    float x_ = 0, vx_ = 0, z_ = 0, vel_ = 0;
    float hold_ = 0, lean_ = 0;
    float brakeIn_ = 0, steerIn_ = 0;
    float shake_ = 0, hornT_ = 0, toneT_ = 0, chimeT_ = 0;
    float limit_ = 18.f;
    char why_[64] = {};
};

}  // namespace luge
