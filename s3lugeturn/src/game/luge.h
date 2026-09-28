// S3 LUGE TURN — hold the sled through three ice bends.
// Steer into the bank. Tip and the run is over.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace luge {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LUGE TURN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int turns() const { return held_; }
    // 0 title, 1 first bend, 2 second, 3 third, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Win, Fail };

    void showTitle();
    void startRun();
    void pilot(float& steer) const;
    void physics(float steer);
    void win();
    void fail(const char* why);
    void blip(float freq);
    void audio();
    void drawRoad();
    void drawWorld();
    void drawPod();
    void drawHud();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog = 0);
    bool project(float lane, float z, float& sx, float& sy, float& scale, int& fog) const;
    float demandAt(float worldS) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    float s_ = 0;
    float speed_ = 0;
    float time_ = 0;
    float steer_ = 0;
    float lean_ = 0;
    float shake_ = 0;
    int held_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
    char why_[64] = {};
};

}  // namespace luge
