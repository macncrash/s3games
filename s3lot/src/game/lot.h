// S3 LOT — three targets, then the gate, before the clock.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace lot {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LOT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int hits() const { return hits_; }
    bool gateOpen() const { return hits_ >= 3; }
    float clockLeft() const { return clock_ / 60.f; }

private:
    enum class Mode { Title, Play, Pause, Win, Lose };

    struct Puff {
        float x, y, t;
    };

    void begin();
    void driveHuman();
    void driveBot();
    void toss();
    void moveBall();
    void celebrate();
    void timeUp();
    bool stepTo(float x, float y);
    void tryMove(float dx, float dy);
    bool solid() const;
    void setFace(float x, float y);
    float targetX(int i) const;
    float targetY(int i) const;
    void blip(float freq, float vol, int frames);
    void chord(float a, float b, float c, int frames);
    void tickAudio();
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float h, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow = false, int fog = 0);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool down_[3] = {};
    float fellX_[3] = {};
    float phase_[3] = {};
    float homeX_[3] = {};
    float homeY_[3] = {};
    int hits_ = 0;
    int clock_ = 0;
    int cool_ = 0;
    int toneLeft_ = 0;
    int chordLeft_ = 0;
    int botTarget_ = 0;
    int botPhase_ = 0;
    float t_ = 0;
    float gateLift_ = 0;
    float px_ = 160, py_ = 196;
    float faceX_ = 0, faceY_ = -1;
    bool ballOn_ = false;
    float bx_ = 0, by_ = 0, bvx_ = 0, bvy_ = 0;
    int ballLife_ = 0;
    std::vector<Puff> puffs_;
};

}  // namespace lot
