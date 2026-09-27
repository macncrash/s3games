#pragma once

// S3 MAZECHIME — walk the hedge until the hour has to chime, then leave.
// The gate stays shut while the clock is still short of twelve.
// Step through it only while the hour is striking.
#include "console/system.h"
#include "game/art.h"

#include <cstdint>
#include <vector>

namespace mazechime {

constexpr int kGraceSec = 20;
constexpr int kFpc = 6;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MAZECHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool walking() const { return mode_ == Mode::Play && steps_ > 0; }
    bool onGate() const { return cx_ == ex_ && cy_ == ey_ && !moving_; }
    int steps() const { return steps_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Play, Chime, Fail, Over };

    void build();
    bool prove();
    bool blocked(int x, int y) const;
    bool gateShut(int x, int y) const;
    void beginRun();
    void arrive();
    void leave();
    void fail(const char* why);
    void readMove();
    void faceDir(int dx, int dy);

    int clockSec() const;
    bool onHour() const;
    bool pastHour() const;
    void split(int& h, int& m, int& s) const;

    void blip(float freq);
    void hush();
    void draw();
    void drawTitle();
    void backdrop();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void put(const gs::Image& img, int x, int y, int pal, bool flip = false, bool shadow = false);
    void center(const gs::Image& img, int y, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    std::vector<uint8_t> hedge_;
    std::vector<int> route_;
    int routeAt_ = 0;
    int sx_ = 1, sy_ = 1;
    int ex_ = 1, ey_ = 1;
    int cx_ = 1, cy_ = 1, tx_ = 1, ty_ = 1;
    int face_ = 0;
    int tick_ = 0;
    int hold_ = 0;
    int steps_ = 0;
    int playFrames_ = 0;
    int toneLeft_ = 0;
    int bellPh_ = 0;
    int lastSec_ = -1;
    int strikes_ = 0;
    bool flip_ = false;
    bool moving_ = false;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool chimed_ = false;
    const char* why_ = "OPEN";
};

}  // namespace mazechime
