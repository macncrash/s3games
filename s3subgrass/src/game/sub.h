// S3 SUB GRASS — beach the boat on the grass shelf and hold a full stop.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace subgrass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SUB GRASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return race_; }
    // 0 title, 1 underway, 2 on the grass, 3 holding the stop, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Run, Win, Fail };

    void showTitle();
    void startRun();
    void controls(float& pitchIn, float& thrust);
    void pilot(float& pitchIn, float& thrust);
    void physics(float pitchIn, float thrust);
    void win();
    void fail(const char* why);
    void draw();
    void hud(float x, float y, const std::string& s, int pal, float scale);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    float bedAt(float x) const;
    float shelfUnder() const;
    bool hullOnGrass() const;
    void blip(float freq);
    void chime(int notes);
    void toneOff();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool grounded_ = false;
    bool launched_ = false;
    float t_ = 0;
    float race_ = 0;
    float x_ = 0, depth_ = 0, pitch_ = 0, speed_ = 0;
    float hold_ = 0;
    float camX_ = 0, camD_ = 0, zoom_ = 4.f;
    float shake_ = 0;
    float tone0_ = 0, tone1_ = 0;
    float chimeT_ = 0;
    int chimeN_ = 0, chimeStep_ = 0;
    int bubbles_ = 0;
    float bubX_[12] = {};
    float bubD_[12] = {};
    float bubA_[12] = {};
    char why_[48] = {};
};

}  // namespace subgrass
