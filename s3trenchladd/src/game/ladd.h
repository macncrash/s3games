// S3 TRENCH LADD — you have the trench. Reach the far ladder. Anything else is a loss.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace trenchladd {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TRENCH LADD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int lives() const { return lives_; }
    float heroX() const { return px_; }
    float climb() const { return climb_; }
    float watchLeft() const { return watch_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the trench, 2 under the gun, 3 the far ladder, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Over };

    struct Span {
        float a, b;
    };
    struct Gun {
        float a, b, period, on, phase;
    };

    void resetRun();
    void begin();
    void bot(bool& left, bool& right, bool& jump, bool& crouch, bool& climb);
    void stepPlay(bool left, bool right, bool jump, bool crouch, bool climb);
    bool gunHot(const Gun& g, float t) const;
    bool inGap(float x) const;
    bool inGun(float x) const;
    bool atLadder() const;
    void win();
    void lose(const char* why);
    void fall();
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);
    const gs::Mipped& hero() const;
    void blip(float freq, float vol, float hold);

    gs::System* sys_ = nullptr;
    Art art_{};
    Span gap_[4]{};
    Gun gun_[3]{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool ducked_ = false;
    bool grounded_ = true;
    int lives_ = 3;
    int face_ = 1;
    const char* reason_ = "";
    float px_ = 0, py_ = 0, vx_ = 0, vy_ = 0;
    float cam_ = 0;
    float t_ = 0, playT_ = 0, step_ = 0;
    float watch_ = 0;
    float climb_ = 0;
    float coyote_ = 0, jumpBuf_ = 0, inv_ = 0;
    float shake_ = 0, beep_ = 0;
    int titleHold_ = 0;
};

}  // namespace trenchladd
