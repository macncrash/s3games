// S3 YARD BANN — one yard. Bring the banner back. Then it is done.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>

namespace yard {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 YARD BANN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int lives() const { return lives_; }
    bool carrying() const { return has_; }
    float heroX() const { return px_; }
    float bannerX() const { return has_ ? px_ : bannerX_; }
    // 0 title, 1 the aisle, 2 the banner is in hand at the hoist, 3 the walk back, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Over };

    struct Dog {
        float x = 0, minX = 0, maxX = 0, speed = 0, dir = 1.f, stun = 0;
    };

    void resetRun();
    void begin();
    void layout();
    void bot(bool& left, bool& right, bool& strike);
    void step(bool left, bool right, bool strike);
    void win();
    void lose();
    void hurt(float fromX);
    void draw();
    void drawWorld(float view);
    void drawPoster();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet, bool shadow = false);
    const gs::Mipped& hero() const;
    void blip(float freq, float vol, float hold);
    void serviceAudio();
    void clearDrop();

    gs::System* sys_ = nullptr;
    Art art_{};
    Dog dogs_[2]{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool has_ = false;
    bool magLow_[2] = {};
    bool jawShut_ = false;
    int lives_ = 4;
    int face_ = 1;
    int fan_ = -1;
    float px_ = 0, vx_ = 0;
    float bannerX_ = 0;
    float cam_ = 0;
    float t_ = 0, playT_ = 0;
    float step_ = 0;
    float strikeCd_ = 0, swing_ = 0;
    float inv_ = 0, stun_ = 0, dropLock_ = 0;
    float shake_ = 0, beep_ = 0;
    float fanT_ = 0;
    float waitT_ = 0, waitX_ = 0;
};

}  // namespace yard
