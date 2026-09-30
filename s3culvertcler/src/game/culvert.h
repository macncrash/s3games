// S3 CULVERT CLER — one culvert. Clear the ground before the clock dies.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace ccler {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CULVERT CLER"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int left() const { return kBits - cleared_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the culvert, 2 the last bits, 3 ground clear, 4 the clock died
    int marker() const;

private:
    static constexpr int kBits = 6;
    enum class Mode { Title, Play, Pause, Won, Lost };

    struct Bit {
        float x = 0, y = 0;
        int kind = 0;
        float work = 0;
        bool gone = false;
    };
    struct Drop {
        float x = 0, y = 0, vy = 0, life = 0;
    };

    void resetFloor();
    void begin();
    void toTitle();
    void updatePlay();
    void botInput(float& ix, float& iy, bool& hold);
    void humanInput(float& ix, float& iy, bool& hold);
    int focusBit() const;
    void win();
    void lose();
    void blip(float freq, float hold);
    void serviceAudio();
    void splash(float x, float y);
    float rnd();

    void draw();
    void vault();
    void world();
    void messages();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet);
    void text(const char* s, float x, float y, float scale, int pal);
    bool startPressed() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Bit bit_[kBits]{};
    Drop drop_[8]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool digging_ = false;
    int focus_ = -1;
    int cleared_ = 0;
    const char* reason_ = "";
    float px_ = 0, py_ = 0;
    float face_ = 1;
    float step_ = 0;
    float t_ = 0;
    float blip_ = 0;
    float tick_ = 0;
    int clock_ = 0;
    uint32_t rng_ = 19;
};

}  // namespace ccler
