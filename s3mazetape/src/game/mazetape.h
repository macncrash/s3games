#pragma once

#include <cstdint>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace mazetape {

// Walk the hedge. The drawer has to match the tape, then the gate lets you leave.
class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MAZETAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const;
    bool rules() const { return rules_; }
    bool held(int i) const;
    int steps() const { return steps_; }
    int traps() const { return traps_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const { return (i >= 0 && i < 3) ? kindName(i) : "?"; }
    int tapeScore(int i) const { return (i >= 0 && i < 3) ? kindPay(i) : 0; }
    const char* reason() const { return reason_; }
    const char* modeName() const;

private:
    enum class Mode { Title, Play, Won };

    struct Spot {
        int x = 0, y = 0;
        int kind = 0;
        bool taken = false;
    };

    void buildWorld();
    void carve();
    bool placeSpots();
    bool buildRoute();
    bool audit();
    int rnd(int n);
    bool wall(int x, int y) const;
    bool blocked(int x, int y) const;
    int openCount(int x, int y) const;
    int spotAt(int x, int y) const;
    bool path(int sx, int sy, int ex, int ey, std::vector<int>& out) const;
    bool tryStep(int nx, int ny);
    void arrive();
    void dropLast();
    void faceDir(int dx, int dy);
    void readMove();
    void updateMotion();
    void draw();
    void drawTitle();
    void backdrop();
    void tileText(int col, int row, const char* s, int pal);
    void put(const gs::Image& img, int x, int y, int pal, bool flip, bool shadow);
    void center(const gs::Image& img, int y, int pal);
    void blip(float freq);
    void thud();
    void chime();
    void quiet();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool won_ = false;
    bool over_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool moving_ = false;
    bool flip_ = false;
    int face_ = 0;
    int sx_ = 1, sy_ = 1, ex_ = 1, ey_ = 1;
    int cx_ = 1, cy_ = 1, tx_ = 1, ty_ = 1;
    int steps_ = 0;
    int tick_ = 0;
    int bump_ = 0;
    int toneLeft_ = 0;
    int routeAt_ = 0;
    int nDrawer_ = 0;
    int traps_ = 0;
    int drawer_[6] = {};
    uint32_t rng_ = 1;
    const char* reason_ = "";
    std::vector<uint8_t> hedge_;
    std::vector<int> route_;
    Spot spots_[6]{};
};

}  // namespace mazetape
