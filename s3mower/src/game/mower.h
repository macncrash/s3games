// S3 MOWER — cut the field before the rain. The stripes are the score.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace mower {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MOWER"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int stripes() const { return stripes_; }
    int goal() const { return kStripes; }
    int left() const { return uncut_; }
    int rainSeconds() const;

    static constexpr int kW = 36;
    static constexpr int kH = 22;
    static constexpr int kStripes = 11;

private:
    struct Pt {
        float x, y;
    };
    struct Drop {
        float x, y, v;
    };
    struct Bit {
        float x, y, vx, vy, t;
    };

    void begin();
    void paintBorder();
    void freshField();
    void paintCell(int r, int c);
    void park();
    void buildPath();
    bool sweep();
    float bandY(int i) const;
    void drive(float& steer, float& gas);
    void botDrive(float& steer, float& gas);
    void move(float steer, float gas);
    void cut();
    void mark(int r, int c, int style);
    void win();
    void lose();
    void sky();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog = 0, bool shadow = false);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void tune();
    void audio();
    void song(int kind);
    float rnd();

    gs::System* sys_ = nullptr;
    Art art_{};
    enum class Mode { Title, Play, Pause, Win, Lose };
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int stripes_ = 0;
    int uncut_ = kW * kH;
    int rainFrames_ = 0;
    int bandLeft_[kStripes] = {};
    bool band_[kStripes] = {};
    uint8_t cell_[kH][kW] = {};
    float x_ = 0, y_ = 0, heading_ = 0, speed_ = 0;
    float storm_ = 0;
    float anchorX_ = 0, anchorY_ = 0;
    float popY_ = 0;
    int pop_ = 0;
    int stuck_ = 0;
    int wp_ = 0;
    int sweeps_ = 0;
    int song_ = -1;
    int songKind_ = 0;
    int flash_ = 0;
    int fresh_ = 0;
    uint32_t rng_ = 0x4d4f57u;
    std::vector<Pt> path_;
    Drop drop_[40] = {};
    Bit bit_[10] = {};
};

}  // namespace mower
