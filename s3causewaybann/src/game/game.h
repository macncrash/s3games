// S3 CAUSEWAY BANN — at the causeway, bring the banner back. Miss that and the watch is over.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace causewaybann {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CAUSEWAY BANN"; }
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
    // 0 title, 1 the outward run, 2 banner in hand, 3 the return, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Over };

    struct Watcher {
        float x, minX, maxX, speed, dir;
    };

    void resetRun();
    void begin();
    void bot(bool& left, bool& right, bool& jump);
    void stepPlay(bool left, bool right, bool jump);
    void win();
    void lose();
    bool solidAt(float x) const;
    bool spanOf(float x, float& a, float& b) const;
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);
    const gs::Mipped& hero() const;
    void blip(float freq, float vol, float hold);

    gs::System* sys_ = nullptr;
    Art art_{};
    Watcher watchers_[2]{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool has_ = false;
    bool faceRight_ = true;
    int lives_ = 4;
    float px_ = 0, py_ = 0, vx_ = 0, vy_ = 0;
    float bannerX_ = 0;
    float cam_ = 0;
    float t_ = 0, playT_ = 0;
    float watch_ = 0;
    float inv_ = 0;
    float safeX_ = 0;
    float toneT_ = 0;
    int titleHold_ = 0;
    bool grounded_ = false;
};

}  // namespace causewaybann
