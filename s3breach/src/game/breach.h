// S3 BREACH — the hall, then the banner. Bring it back through the door you broke.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace breach {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BREACH"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int lives() const { return lives_; }
    int hits() const { return hits_; }
    bool carrying() const { return has_; }
    float heroX() const { return px_; }
    float bannerX() const { return bannerX_; }
    float gate() const { return alarm_; }
    // 0 title, 1 the hall, 2 the banner, 3 the return, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Over, Victory };

    struct Guard {
        float x, minX, maxX, speed, dir;
    };

    void resetRun();
    void begin();
    void bot(bool& left, bool& right, bool& down, bool& jump);
    void stepPlay(float dt, bool left, bool right, bool down, bool jump);
    void hurt(float fromX);
    void win();
    void lose();
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet = false, bool shadow = false);
    void world(const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip, bool feet = true);
    const gs::Mipped& hero() const;
    void blip(float freq, float vol, float hold);
    void noteOff();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Guard guards_[3]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool has_ = false;
    bool alarmOn_ = false;
    bool grounded_ = false;
    bool ducked_ = false;
    int lives_ = 3;
    int hits_ = 0;
    int face_ = 1;
    float px_ = 0, py_ = 0, vx_ = 0, vy_ = 0;
    float bannerX_ = 0;
    float alarm_ = 0;
    float cam_ = 0;
    float t_ = 0;
    float coyote_ = 0, jumpBuf_ = 0;
    float inv_ = 0, stun_ = 0, dropLock_ = 0;
    float shake_ = 0, step_ = 0, beep_ = 0;
    float fanT_ = 0;
    int fan_ = -1;
};

}  // namespace breach
