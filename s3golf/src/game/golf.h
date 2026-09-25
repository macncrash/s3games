// S3 GOLF — three holes. In the hole, not on the green.
#pragma once

#include <string>

#include "console/system.h"
#include "game/art.h"

namespace golf {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GOLF"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int cups() const { return cups_; }
    int strokes() const { return strokes_; }
    int holeNo() const { return hole_ + 1; }
    float ballX() const { return ball_.x; }
    const char* lie() const { return lie_; }

private:
    struct Ball {
        float x = 0, y = 0, vx = 0, vy = 0;
        float prevX = 0, prevY = 0;
        int kind = 0;
        bool rolling = false;
        bool rest = true;
        bool holed = false;
        bool wet = false;
        bool ob = false;
        bool inWell = false;
        bool skipped = false;
    };
    struct Shot {
        int club = 2;
        float ang = 0.8f;
        float spd = 200;
    };
    struct SimOut {
        bool holed = false;
        bool wet = false;
        bool ob = false;
        bool onGreen = false;
        bool putt = false;
        float x = 0;
        int kind = 0;
    };

    void startRound();
    void loadHole();
    void place(Ball& b, float x) const;
    void launch(Ball& b, int club, float ang, float spd) const;
    void tick(Ball& b, int club, float dt) const;
    bool belowLip(const Ball& b) const;
    SimOut simulate(const Ball& from, int club, float ang, float spd) const;
    float crossX(const Ball& from, int club, float ang, float spd) const;
    float bisect(const Ball& from, int club, float ang, float target) const;
    void swing(int club, float ang, float spd);
    void holeOut();
    void replay(const char* msg, int pal);
    void settle();
    Shot plan() const;
    float shotAngle(int club, float aim) const;
    void say(const char* s, int pal, float time);
    void chime(float dt);
    void draw();
    void backdrop();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Ball ball_{};
    const char* lie_ = "TEE";
    int mode_ = 0;  // 0 title, 1 play, 2 pause, 3 banner, 4 win, 5 lose
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool needSettle_ = false;
    int hole_ = 0;
    int cups_ = 0;
    int strokes_ = 0;
    int holeStrokes_ = 0;
    int best_ = 0;
    int club_ = 2;
    int flyClub_ = 2;
    int chime_ = 0;
    int sayPal_ = 1;
    int shotFrames_ = 0;
    int swingT_ = 0;
    float aim_ = 0.7f;
    float meter_ = 0;
    float meterDir_ = 1;
    float t_ = 0;
    float bannerT_ = 0;
    float sayT_ = 0;
    float chimeT_ = 0;
    float toneKill_ = 0;
    float lieX_ = 0;
    bool charging_ = false;
    std::string say_;
};

}  // namespace golf
