// S3 TOWER BANN — one tower. Bring the banner back. Then it is done.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace towerbann {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TOWER BANN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int lives() const { return lives_; }
    bool carrying() const { return has_; }
    int floor() const { return floor_; }
    float heroY() const { return py_; }
    // 0 title, 1 the climb, 2 banner in hand, 3 the way down, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Over };

    void resetRun();
    void begin();
    void bot(bool& left, bool& right, bool& jump);
    void stepPlay(bool left, bool right, bool jump);
    void win();
    void lose();
    void peck();
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);
    const gs::Mipped& hero() const;
    void blip(float freq, float vol, float hold);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool has_ = false;
    bool grounded_ = false;
    int lives_ = 4;
    int face_ = 1;
    int floor_ = 0;
    int bannerFloor_ = 0;
    float px_ = 0, py_ = 0, vx_ = 0, vy_ = 0;
    float bannerX_ = 0;
    float crowX_ = 0, crowDir_ = 1;
    float camBottom_ = 0;
    float airTargetX_ = 0;
    float t_ = 0, playT_ = 0;
    float inv_ = 0, stun_ = 0, shake_ = 0, beep_ = 0;
    int titleHold_ = 0;
};

}  // namespace towerbann
