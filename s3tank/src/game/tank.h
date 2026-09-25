// S3 TANK — one street, two tanks. The one still moving takes the block.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace tank {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TANK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int hits() const { return hits_; }
    const char* why() const { return why_; }
    // 0 title, 1 the street, 2 the block is taken, 3 stopped
    int marker() const;

private:
    enum class Mode { Title, Duel, Pause, Won, Lost };

    struct Body {
        float x = 0, z = 0, speed = 0, stall = 0;
        int face = 1;
        bool stopped = false;
    };
    struct Shell {
        float x = 0, z = 0, vz = 0;
        bool player = false;
        bool live = true;
    };
    struct Puff {
        float x = 0, z = 0, t = 0, h = 1;
    };
    struct Prop {
        float x = 0, z = 0;
        int kind = 0;
    };

    void beginDuel();
    void update(float dt);
    void draw();
    void think(float& steer, bool& fire);
    void launch(bool player);
    void damage(Body& b, bool player);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow = false);
    void blip(bool high);
    void boom();
    void fanfare();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int round_ = 0;
    int hits_ = 0;
    int botSide_ = -1;
    const char* why_ = "still rolling";
    float t_ = 0;
    float weave0_ = 0.4f;
    float fireCd_ = 0;
    float rivalFire_ = 0.7f;
    float flashP_ = 0, flashR_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    bool engineOn_ = false;
    int fanStep_ = -1;
    float fanT_ = 0;
    Body player_{};
    Body rival_{};
    std::vector<Shell> shells_;
    std::vector<Puff> puffs_;
    std::vector<Prop> props_;
};

}  // namespace tank
