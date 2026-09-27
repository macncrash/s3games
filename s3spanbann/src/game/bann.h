// S3 SPAN BANN — at the span, bring the banner back. Miss that and the watch is over.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace sbann {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SPAN BANN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool carrying() const { return carrying_; }
    float heroX() const { return px_; }
    float bannerX() const { return bx_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the crossing, 2 the banner is in hand on the far side, 3 the return, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };

    struct Puff {
        float x = 0, y = 0, t = 0;
        int kind = 0;
    };

    void boot();
    void begin();
    void resetWorld();
    void step(float dir, bool jump, bool act);
    void bot(float& dir, bool& jump, bool& act) const;
    void grab();
    void plant();
    void dropBanner(float x);
    void knock(float fromX);
    void fall();
    void win();
    void lose(const char* why);
    void blip(float freq);
    void audio();
    float clock() const;
    float windNow() const;
    float lowness() const;
    bool chainOpen() const;
    bool solid(float x) const;
    int segAt(float x) const;
    float deckY(float x) const;
    float cableY(float x) const;
    float snap(float x) const;
    bool gapJump(int way) const;
    bool sentryJump(int way) const;
    bool chainWait(int way) const;
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0, bool feet = false);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool carrying_ = false;
    bool planted_ = false;
    bool onGround_ = true;
    const char* reason_ = "THE WATCH IS OVER";
    int face_ = 1;
    int lastSec_ = 99;
    float t_ = 0;
    float playT_ = 0;
    float watch_ = 0;
    float px_ = 0;
    float py_ = 0;
    float vx_ = 0;
    float vy_ = 0;
    float bx_ = 0;
    float by_ = 0;
    float lastSolid_ = 0;
    float stun_ = 0;
    float coyote_ = 0;
    float jumpBuf_ = 0;
    float step_ = 0;
    float shake_ = 0;
    float blipT_ = 0;
    float blipF_ = 0;
    float splash_ = 0;
    std::vector<Puff> puffs_;
};

}  // namespace sbann
