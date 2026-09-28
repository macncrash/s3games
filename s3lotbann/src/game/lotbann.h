// S3 LOTBANN — the lot is yours. Bring the banner back. Anything else is a loss.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace lotbann {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LOTBANN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int lives() const { return lives_; }
    bool carrying() const { return has_; }
    float heroX() const { return px_; }
    float heroY() const { return py_; }
    float clockLeft() const { return clock_; }
    // 0 title, 1 out on the lot, 2 banner in hand on the far side, 3 the walk back, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Over };

    struct Box {
        float x, y, w, h;
    };
    struct Lane {
        float y0, y1, dir, speed;
        float carX[2];
    };

    void resetRun();
    void begin();
    void bot(bool& left, bool& right, bool& up, bool& down);
    void stepPlay(bool left, bool right, bool up, bool down);
    bool blocked(float x, float y) const;
    bool laneUnsafe(int lane) const;
    bool crossBlocked(float yGoal) const;
    void hit(int lane);
    void win();
    void lose();
    void blip(float freq, float vol, float hold);
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip);

    gs::System* sys_ = nullptr;
    Art art_{};
    std::vector<Box> stall_;
    Lane lane_[4]{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool has_ = false;
    int lives_ = 3;
    int face_ = 1;
    float px_ = 0, py_ = 0;
    float camX_ = 0, camY_ = 0;
    float t_ = 0, playT_ = 0;
    float clock_ = 0;
    float inv_ = 0, beep_ = 0, shake_ = 0;
    int titleHold_ = 0;
};

}  // namespace lotbann
