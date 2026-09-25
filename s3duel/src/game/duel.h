// S3 DUEL — three paces, then the draw. A shot before DRAW loses.
#pragma once

#include <string>

#include "console/system.h"
#include "game/art.h"

namespace duel {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DUEL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* reason() const { return reason_; }
    int phase() const;
    int pace() const { return pace_; }

private:
    enum class Mode { Title, Face, Pace, Turn, Holster, Draw, Resolve, Over };

    struct Fighter {
        float x = 0;
        int pose = POSE_FACE;
        bool flip = false;
    };

    void openDuel();
    void beginPaces();
    void foul();
    void cleanShot();
    void slowShot();
    void stepPace();
    void stepTurn();
    void stepResolve();
    void audioTick();
    void tone(int ch, float freq, float vol, float hold);
    void layStreet();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet = false, bool shadow = false);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    bool fireEdge() const;
    bool startEdge() const;
    uint32_t rnd();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Fighter you_{}, cole_{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool early_ = false;
    bool settled_ = false;
    bool playerWon_ = false;
    const char* reason_ = "unfinished";
    int t_ = 0;
    int titleT_ = 0;
    int pace_ = 0;
    int foot_ = 0;
    int holsterWait_ = 40;
    int rivalDraw_ = 30;
    int flash_ = 0;
    int puff_ = 0;
    float shake_ = 0;
    float hold_[3] = {};
    uint32_t rng_ = 0xD0E15A11u;
};

}  // namespace duel
