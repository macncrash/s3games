#pragma once

// S3 MAZEBELL — a short hedge and a bell.
// The bell rings only when you stand in its cell. A pit or the false bell
// kills the try. The third dead try ends it. Leave when the bell has rung.
#include "console/system.h"
#include "game/art.h"

#include <cstdint>
#include <vector>

namespace mazebell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MAZEBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const { return rules_; }
    bool walking() const { return mode_ == Mode::Play && steps_ > 0; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }
    int steps() const { return steps_; }
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Play, Dead, Ring, Over };

    void build();
    bool routeAround();
    bool prove();
    bool blocked(int x, int y) const;
    bool deathAt(int x, int y) const;
    void beginTry();
    void arrive();
    void ring();
    void dieTry();
    void readMove();
    void faceDir(int dx, int dy);

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
    int bx_ = 1, by_ = 1;
    int fx_ = 1, fy_ = 1;
    int px_ = 1, py_ = 1;
    int cx_ = 1, cy_ = 1, tx_ = 1, ty_ = 1;
    int face_ = 0;
    int tick_ = 0;
    int hold_ = 0;
    int steps_ = 0;
    int dead_ = 0;
    int tryNo_ = 1;
    int toneLeft_ = 0;
    int bellPh_ = 0;
    bool flip_ = false;
    bool moving_ = false;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool rules_ = false;
    const char* why_ = "OPEN";
};

}  // namespace mazebell
