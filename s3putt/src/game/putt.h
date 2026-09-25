// S3 PUTT — nine short holes. Only the cup counts.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace putt {

struct Body {
    float x = 0, y = 0, vx = 0, vy = 0;
    bool sunk = false;
    bool wet = false;
    bool rest = true;
    int bumps = 0;
};

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PUTT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int cups() const { return cups_; }
    int strokes() const { return strokes_; }
    const char* holeName() const;
    float ballX() const { return ball_.x; }
    float ballY() const { return ball_.y; }

private:
    enum class Mode { Title, Play, Pause, Banner, Win };

    void startRound();
    void loadHole();
    void strike(float ang, float spd);
    void stepBody(Body& b, float dt);
    bool findPutt(float x, float y, float& ang, float& spd);
    void holeOut();
    void splash();
    void chime(float dt);
    void say(const char* s, int pal, float time);
    void draw();
    void backdrop();
    void patch(const gs::Mipped& m, float x, float y, float w, float h, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);
    void flagAt(float wx, float wy);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Body ball_{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool swinging_ = false;
    bool rolled_ = false;
    int hole_ = 0;
    int cups_ = 0;
    int strokes_ = 0;
    int best_ = 0;
    int chime_ = 0;
    int rollFrames_ = 0;
    int sayPal_ = PAL_GOLD;
    float aim_ = -1.5707963f;
    float meter_ = 0;
    float meterDir_ = 1;
    float t_ = 0;
    float bannerT_ = 0;
    float sayT_ = 0;
    float chimeT_ = 0;
    float toneKill_ = 0;
    std::string say_;
};

}  // namespace putt
