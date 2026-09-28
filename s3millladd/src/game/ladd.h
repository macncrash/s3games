// S3 MILL LADD — one mill. Reach the far ladder. Then it is done.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace millladd {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MILL LADD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    // 0 title, 1 the race, 2 the hopper, 3 the far ladder, 4 ended
    int marker() const;
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Play, Pause, Won, Lost };

    void begin();
    void win();
    void lose(const char* why);
    void paintWall();
    void play(float dt, bool left, bool right, bool up, bool down, bool jumpPressed);
    void draw();
    void audio();
    void bot(bool& left, bool& right, bool& up, bool& down, bool& jump) const;
    void hud(int col, int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);
    void world(const gs::Mipped& m, float wx, float wy, float h, int pal, bool flip, bool feet);
    int nearLadder() const;
    bool onGallery() const;
    float hopperX() const;
    const gs::Mipped& heroSprite() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool grounded_ = false;
    bool onLadder_ = false;
    int lad_ = -1;
    int face_ = 1;
    int fan_ = -1;
    const char* reason_ = "";
    float t_ = 0;
    float px_ = 64, py_ = 188, vx_ = 0, vy_ = 0;
    float cam_ = 0;
    float coyote_ = 0, jumpBuf_ = 0;
    float step_ = 0, foot_ = 0, climbSnd_ = 0, beep_ = 0, fanT_ = 0;
    int botPhase_ = 0;
};

}  // namespace millladd
