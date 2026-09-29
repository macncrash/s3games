// S3 TRENCH BANN — one trench. Bring the banner back. Then it is done.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace trenchbann {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TRENCH BANN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int lives() const { return lives_; }
    bool carrying() const { return has_; }
    float heroX() const { return px_; }
    float bannerX() const { return has_ ? px_ : bannerX_; }
    float watchLeft() const { return watch_; }
    // 0 title, 1 the walk out, 2 banner in hand still deep, 3 the walk back, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Over };

    struct Flare {
        float a = 0, b = 0, period = 1, phase = 0, on = 0.8f;
    };

    void resetRun();
    void begin();
    void bot(bool& left, bool& right, bool& crouch);
    void stepPlay(bool left, bool right, bool crouch);
    bool flareHot(const Flare& f, float t) const;
    bool inMud(float x) const;
    void win();
    void lose();
    void stumble(float fromX);
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);
    const gs::Mipped& hero() const;
    void blip(float freq, float vol, float hold);

    gs::System* sys_ = nullptr;
    Art art_{};
    Flare flare_[3]{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool has_ = false;
    bool ducked_ = false;
    int lives_ = 4;
    int face_ = 1;
    float px_ = 0, vx_ = 0;
    float bannerX_ = 0;
    float cam_ = 0;
    float t_ = 0, playT_ = 0, step_ = 0;
    float watch_ = 0;
    float inv_ = 0, stun_ = 0, dropLock_ = 0;
    float shake_ = 0, beep_ = 0;
    int titleHold_ = 0;
};

}  // namespace trenchbann
