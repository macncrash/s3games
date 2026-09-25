// S3 EAVES — night roofs. The far ladder is the only way off. Don't fall.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace eaves {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 EAVES"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int roof() const { return route_ + 1; }
    int falls() const { return falls_; }
    float heroX() const { return px_; }
    float heroY() const { return py_; }

private:
    enum Mode { Title, Play, Pause, Drop, Over, Victory };
    struct Puff {
        float x, y, life;
    };

    void resetRun();
    void paintCity();
    void play(float dt, bool left, bool right, bool up, bool down, bool jumpPressed);
    void bot(bool& left, bool& right, bool& up, bool& down, bool& jump) const;
    void mount(int i);
    void leave(bool top);
    int platAt(float x, float y) const;
    void miss();
    void win();
    void respawn();
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet = false);
    void world(const gs::Mipped& m, float wx, float wy, float h, int pal, bool flip, bool feet = false);
    const gs::Mipped& hero() const;
    void blip(float freq, float vol, float hold);
    void noteOff();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool announced_ = false;
    bool grounded_ = false;
    bool onLadder_ = false;
    int route_ = 0;
    int onPlat_ = 0;
    int checkpoint_ = 0;
    int tries_ = 5;
    int falls_ = 0;
    int lad_ = 0;
    int face_ = 1;
    float t_ = 0;
    float px_ = 0, py_ = 0, vx_ = 0, vy_ = 0;
    float camX_ = 0, camY_ = 0;
    float coyote_ = 0, jumpBuf_ = 0, lockout_ = 0;
    float dropT_ = 0, beep_ = 0, step_ = 0, foot_ = 0, climbSnd_ = 0;
    float fanT_ = 0;
    int fan_ = -1;
    std::vector<Puff> puffs_;
};

}  // namespace eaves
