#pragma once

#include <cstdint>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace maze {

// A hedge stands off the path. Reaching the exit is the only win.
class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MAZE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_ && cx_ == ex_ && cy_ == ey_; }
    int steps() const { return steps_; }

private:
    enum class Mode { Title, Play, Won };

    void buildWorld();
    void carve();
    void pickDecoy();
    bool buildRoute();
    int rnd(int n);
    bool blocked(int x, int y) const;
    int openCount(int x, int y) const;
    bool tryStep(int nx, int ny);
    void faceDir(int dx, int dy);
    void readMove();
    void updateMotion();
    void resetRun();
    void draw();
    void drawTitle();
    void backdrop();
    void glow();
    void tileText(int col, int row, const char* s);
    void put(const gs::Image& img, int x, int y, int pal, bool flip, bool shadow);
    void center(const gs::Image& img, int y, int pal);
    void blip();
    void thud();
    void chime();
    void quiet();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool won_ = false;
    bool over_ = false;
    bool moving_ = false;
    bool flip_ = false;
    int face_ = 0;
    int sx_ = 1, sy_ = 1, ex_ = 1, ey_ = 1;
    int cx_ = 1, cy_ = 1, tx_ = 1, ty_ = 1;
    int decoyX_ = -1, decoyY_ = -1;
    int steps_ = 0;
    int tick_ = 0;
    int bump_ = 0;
    int toneLeft_ = 0;
    int routeAt_ = 0;
    uint32_t rng_ = 1;
    std::vector<uint8_t> hedge_;
    std::vector<int> route_;
};

}  // namespace maze
