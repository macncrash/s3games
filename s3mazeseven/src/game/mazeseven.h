#pragma once

#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace mazeseven {

// One lamp apiece. The number on the lamp is the tally.
// First to seven wins. Six is still short, and the hedge has no exit.
class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MAZE SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const;
    int you() const { return you_; }
    int them() const { return them_; }
    int steps() const { return steps_; }

private:
    enum class Mode { Title, Play, Show, Win, Lose };
    enum class Who { You, Them };

    struct Lamp {
        int x = 0;
        int y = 0;
        int val = 0;
        bool live = false;
    };
    struct Walker {
        int x = 1;
        int y = 1;
        int tx = 1;
        int ty = 1;
        int face = 0;
        int tick = 0;
        bool flip = false;
        bool moving = false;
    };

    void buildMap();
    void clearMatch();
    void beginMatch();
    void beginActor();
    bool pathTo(int sx, int sy, int gx, int gy);
    int nextTarget(bool yours) const;
    int lampAt(int x, int y) const;
    bool blocked(int x, int y) const;
    int pace() const { return bot_ ? 4 : 8; }
    Walker& active() { return who_ == Who::You ? self_ : rival_; }
    void face(Walker& w, int dx, int dy);
    bool startStep(Walker& w, int nx, int ny);
    void stepWalker(Walker& w);
    void play();
    void readHuman();
    void claim(int id);
    void finish(bool win);

    void blip(float freq);
    void hush();
    void draw();
    void drawTitle();
    void drawPlay();
    void backdrop();
    void flicker();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void put(const gs::Image& img, int x, int y, int pal, bool flip = false, bool shadow = false);
    void center(const gs::Image& img, int y, int pal);
    void walker(const Walker& w, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Lamp lamps_[8]{};
    int lampN_ = 0;
    int youBook_[3] = {-1, -1, -1};
    int themBook_[2] = {-1, -1};
    bool booksOk_ = false;
    std::vector<uint8_t> hedge_;
    std::vector<int> route_;
    int routeAt_ = 0;
    int target_ = -1;

    Mode mode_ = Mode::Title;
    Who who_ = Who::You;
    Walker self_{};
    Walker rival_{};
    bool bot_ = false;
    bool won_ = false;
    bool over_ = false;
    bool sawShort_ = false;
    int you_ = 0;
    int them_ = 0;
    int nYou_ = 0;
    int nThem_ = 0;
    int youTake_[8] = {};
    int themTake_[8] = {};
    int steps_ = 0;
    int hold_ = 0;
    int bump_ = 0;
    int toneLeft_ = 0;
    int yx_ = 1;
    int yy_ = 1;
    int tx_ = 9;
    int ty_ = 1;
};

}  // namespace mazeseven
