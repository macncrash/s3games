// S3 DEPOT LADD — at the depot, reach the far ladder. Miss that and the watch is over.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace depotladd {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DEPOT LADD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* reason() const { return reason_; }
    float heroX() const { return px_; }
    float heroY() const { return py_; }
    int heroPlat() const { return onPlat_; }
    // 0 title, 1 the yard, 2 the tank vent, 3 the gantry, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Won, Lost };
    struct Puff {
        float x, y, life;
    };

    void begin();
    void win();
    void lose(const char* why);
    void paint();
    void bot(bool& left, bool& right, bool& up, bool& down, bool& jump);
    void play(float dt, bool left, bool right, bool up, bool down, bool jumpPressed);
    void draw();
    void audio();
    void blip(float freq, float vol, float hold);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet, bool shadow = false);
    void world(const gs::Mipped& m, float wx, float wy, float h, int pal, bool flip, bool feet, bool shadow = false);
    void text(const char* s, float x, float y, float scale, int pal);
    void label(const char* s, float wx, float wy, float scale, int pal);
    void mount(int i);
    void leave(bool top);
    int nearLadder(bool down) const;
    const gs::Mipped& heroSprite() const;
    float viewX() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool grounded_ = false;
    bool onLadder_ = false;
    int onPlat_ = 0;
    int lad_ = -1;
    int face_ = 1;
    int fan_ = -1;
    int tickSec_ = -1;
    const char* reason_ = "";
    float t_ = 0;
    float px_ = 0, py_ = 0, vx_ = 0, vy_ = 0;
    float camX_ = 0, camY_ = 0;
    float coyote_ = 0, jumpBuf_ = 0, lock_ = 0;
    float beep_ = 0, foot_ = 0, step_ = 0, climbSnd_ = 0;
    float shake_ = 0, fanT_ = 0;
    std::vector<Puff> puffs_;
};

}  // namespace depotladd
