// S3 FOUNDRY BANN — you have the foundry. Bring the banner back.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace foundry {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FOUNDRY BANN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int lives() const { return lives_; }
    bool carrying() const { return carry_; }
    float heroX() const { return px_; }
    float bannerX() const { return carry_ ? px_ : bx_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the floor, 2 the banner is in hand, 3 the return, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };

    struct Pour {
        float x = 0;
        float phase = 0;
    };
    struct Roll {
        float x = 0;
        float vx = 0;
    };

    void bootPlay();
    void update(float dt);
    void botThink();
    void tryGrab();
    void hurt();
    bool pourHot(const Pour& p, float t) const;
    bool blocked(float dir) const;
    void win();
    void lose(const char* why);
    void blip(float freq);
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float foot, float h, int pal, bool flip);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool carry_ = false;
    bool onGround_ = true;
    const char* reason_ = "THE BANNER IS STILL OUT";
    int lives_ = 3;
    int face_ = 1;
    float t_ = 0;
    float px_ = 120;
    float py_ = 190;
    float vx_ = 0;
    float vy_ = 0;
    float bx_ = 1960;
    float inv_ = 0;
    float cam_ = 0;
    std::vector<Pour> pours_;
    std::vector<Roll> rolls_;
};

}  // namespace foundry
