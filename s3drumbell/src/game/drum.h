// S3 DRUMBELL — four strokes on a short drum. The bell rings before the third try dies.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace drumbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DRUMBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    int dead() const { return dead_; }
    int attempt() const { return attempt_; }
    const char* why() const { return why_; }

private:
    enum class Mode { Title, Play, Gap, Rise, Over };

    void begin();
    void resetTry();
    void strike();
    void killTry(const char* why);
    void ring();
    bool botTap() const;
    float beaterX() const;
    int hits() const;
    void audio();
    void blip(float freq, float vol, int frames);
    void draw();
    void backdrop();
    void image(const gs::Mipped& m, float left, float top, float h, int pal, bool flip = false);
    void spr(const gs::Mipped& m, float cx, float bottom, float h, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool did_[kMarks] = {};
    int dead_ = 0;
    int attempt_ = 0;
    int anim_ = 0;
    int swing_ = 0;
    int shake_ = 0;
    int blip_ = 0;
    int fanT_ = 0;
    float phase_ = 0;
    const char* why_ = "";
    char say_[24] = {};
};

}  // namespace drumbell
