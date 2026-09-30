// S3 METRO PASS — one train. Clear the pass before the storm clock.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace metropass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 METRO PASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return tRun_; }
    // 0 title, 1 the pass, 2 ended
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Result };

    struct Block {
        float z;
        int side;  // -1 left rail, +1 right rail
        bool hit;
    };
    struct Flake {
        float x, y, life, s;
    };
    struct Proj {
        float x = 0, y = 0, z = 0, hw = 0, fog = 0;
        bool ok = false;
    };

    void resetRide();
    void beginRun();
    void botDrive(int& rail, float& power, float& brake) const;
    void humanDrive(const gs::Pad& pad, int& rail, float& power, float& brake) const;
    void integrate(int rail, float power, float brake);
    void endRun(int why);
    Proj project(float wz, float wx) const;
    void blit(const gs::Mipped& m, float cx, float foot, float h, int pal, bool flip, int fog);
    void ui(const gs::Image& im, float x, float y, int pal);
    void hudText(int col, int row, const char* s, int pal);
    void sky();
    void road();
    void world();
    void hud();
    void audio();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int why_ = 0;
    int rail_ = -1;
    float clock_ = 0;
    float t_ = 0;
    float tRun_ = 0;
    float z_ = 0;
    float x_ = 0;
    float speed_ = 0;
    float power_ = 0;
    float brake_ = 0;
    float shake_ = 0;
    int hor_ = 96;
    std::vector<Block> blocks_;
    std::vector<Flake> flakes_;
};

}  // namespace metropass
