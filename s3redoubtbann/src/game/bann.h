// S3 REDOUBT BANN — at the redoubt, bring the banner back.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace redoubtbann {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 REDOUBT BANN"; }
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
    // 0 title, 1 the approach, 2 banner in hand past the wire, 3 the walk back, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Over };

    struct Raider {
        float x = 0, minX = 0, maxX = 0, speed = 0, dir = 1;
        int lane = 0;
    };
    struct Gabion {
        float x = 0;
        int lane = 0;
    };

    void resetRun();
    void begin();
    void bot(bool& left, bool& right, bool& up, bool& down, bool& hop);
    void stepPlay(bool left, bool right, bool up, bool down, bool hop);
    bool blocked(int lane, float x, int dir, float reach) const;
    void win();
    void lose();
    void hit(float fromX);
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float feet, float h, int pal, bool flip);
    float laneFeet(int lane) const;
    void blip(float freq, float vol, float hold);

    gs::System* sys_ = nullptr;
    Art art_{};
    Raider raider_[5]{};
    Gabion gabion_[9]{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool has_ = false;
    int lives_ = 4;
    int face_ = 1;
    int lane_ = 1;
    int bannerLane_ = 2;
    float px_ = 0, vx_ = 0;
    float bannerX_ = 0;
    float cam_ = 0;
    float t_ = 0, playT_ = 0;
    float watch_ = 0;
    float hop_ = 0, hopCd_ = 0, inv_ = 0, laneCd_ = 0;
    float shake_ = 0, beep_ = 0;
};

}  // namespace redoubtbann
