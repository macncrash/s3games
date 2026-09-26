// S3 MAZEMARK — one short hedge. The gold coin is the mark.
// Standing on the coin opens it. Lifting the coin finishes it.
// That finished mark ends the cartridge. The lit gate is not the job.
#pragma once

#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace mazemark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MAZEMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool lifted() const { return lifted_; }
    bool onMark() const { return cx_ == mx_ && cy_ == my_ && !moving_; }
    int steps() const { return steps_; }
    int leaves() const { return leaves_; }

private:
    enum class Mode { Title, Play, Lift, Pause, Win, Lose };

    void buildWorld();
    void buildRoute();
    bool blocked(int x, int y) const;
    void resetRun();
    void toTitle();
    void begin();
    void startLift();
    void finish();
    void fail();
    void faceDir(int dx, int dy);
    bool tryStep(int nx, int ny);
    void updateMotion();
    void updateLift();
    void readMove();
    void backdrop();
    void pulse();
    void draw();
    void drawTitle();
    void drawWorld();
    void blip(float freq, float vol, int hold);
    void chord();
    void quiet();
    void put(const gs::Image& img, int x, int y, int w, int h, int pal, bool flip = false, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Play;
    bool bot_ = false;
    bool won_ = false;
    bool over_ = false;
    bool lifted_ = false;
    bool moving_ = false;
    bool flip_ = false;
    int face_ = 0;
    int sx_ = 1, sy_ = 1, ex_ = 1, ey_ = 1, mx_ = 1, my_ = 1;
    int cx_ = 1, cy_ = 1, tx_ = 1, ty_ = 1;
    int steps_ = 0;
    int leaves_ = 0;
    int tick_ = 0;
    int bump_ = 0;
    int liftT_ = 0;
    int wait_ = 0;
    int toneLeft_ = 0;
    int routeAt_ = 0;
    std::vector<uint8_t> hedge_;
    std::vector<int> route_;
};

}  // namespace mazemark
