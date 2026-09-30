// S3 GRANARY LADD — you have the granary. Reach the far ladder. Anything else is a loss.
#pragma once
#include <vector>

#include "console/gfx.h"
#include "console/system.h"

namespace granaryladd {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GRANARY LADD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    // 0 title, 1 the bay, 2 the rolling barrel, 3 the loft, 4 ended
    int marker() const;
    const char* reason() const { return reason_; }
    float heroX() const { return px_; }
    float heroY() const { return py_; }

private:
    enum class Mode { Title, Play, Pause, Won, Lost };

    struct Puff {
        float x, y, life;
    };

    void buildArt();
    void paint();
    void begin();
    void win();
    void lose(const char* why);
    void bot(bool& left, bool& right, bool& up, bool& down, bool& jump) const;
    void play(float dt, bool left, bool right, bool up, bool down, bool jumpHeld);
    void draw();
    void audio();
    void blip(float freq, float vol, float hold);
    void textAt(const char* s, float x, float y, int pal);
    void spr(const gs::Mipped& m, float cx, float feetY, float h, int pal, bool flip);
    void worldSpr(const gs::Mipped& m, float wx, float feetY, float h, int pal, bool flip);
    float barrelX() const;
    float viewX() const;
    int nearLadder() const;

    gs::System* sys_ = nullptr;
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool grounded_ = false;
    bool onLadder_ = false;
    int onPlat_ = 0;
    int face_ = 1;
    const char* reason_ = "";
    float t_ = 0;
    float px_ = 0, py_ = 0, vx_ = 0, vy_ = 0;
    float camX_ = 0, camY_ = 0;
    float coyote_ = 0, jumpBuf_ = 0, lock_ = 0;
    float beep_ = 0, step_ = 0, foot_ = 0, climbSnd_ = 0;
    float shake_ = 0;
    int tickSec_ = -1;
    std::vector<Puff> puffs_;

    gs::Mipped stand_, walkA_, walkB_, jump_, climbA_, climbB_;
    gs::Mipped ladder_, barrel_, sack_, mote_;
    gs::Image glyphs_[96] = {};
    int plank_ = 1, post_ = 1, wheat_ = 1, pit_ = 1, beam_ = 1, loft_ = 1;
};

}  // namespace granaryladd
