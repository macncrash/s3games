// S3 LUGE KILO — take the luge down the ice kilometer.
// Do not touch a wheel. The clock on the board is the other crew.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace luge {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LUGE KILO"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float meters() const { return s_; }
    float seconds() const { return time_; }
    // 0 title, 1 on the ice, 2 a wheel is close, 3 the last stretch, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Win, Fail };

    struct Puff {
        float x, y, t;
    };

    void showTitle();
    void startRun();
    void pilot(float& steer, float& tuck) const;
    void physics(float steer, float tuck);
    void win();
    void fail(const char* why);
    void blip(float freq);
    void audio();
    void drawRoad();
    void drawWorld();
    void drawLuge();
    void drawHud();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false, int fog = 0);
    bool project(float lane, float z, float& sx, float& sy, float& scale, int& fog) const;
    float bendAt(float worldS) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    float s_ = 0;
    float x_ = 0;
    float speed_ = 0;
    float time_ = 0;
    float steer_ = 0;
    float lean_ = 0;
    float shake_ = 0;
    float wind_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
    int puffN_ = 0;
    Puff puffs_[12]{};
    char why_[64] = {};
};

}  // namespace luge
