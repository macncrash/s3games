// S3 QUARRY CLER — you have the quarry. Clear the ground before the clock dies.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace qcler {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 QUARRY CLER"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int left() const { return kLoads - dumped_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the pit, 2 the last loads, 3 ground clear, 4 the clock died
    int marker() const;

private:
    static constexpr int kLoads = 6;
    enum class Mode { Title, Play, Pause, Won, Lost };

    struct Load {
        float x = 0, y = 0;
        int kind = 0;
        float work = 0;
        bool gone = false;
    };
    struct Mote {
        float x = 0, y = 0, vx = 0, vy = 0, life = 0;
    };

    void resetPit();
    void begin();
    void toTitle();
    void updatePlay();
    void botInput(float& ix, float& iy, bool& hold);
    void humanInput(float& ix, float& iy, bool& hold);
    void move(float ix, float iy);
    void updateWork(bool hold);
    int focusLoad() const;
    bool atHopper() const;
    void win();
    void lose();
    void blip(float freq, float hold);
    void serviceAudio();
    void fadeMotes();
    void puff(float x, float y);
    float rnd();

    void draw();
    void pit();
    void scenery();
    void loads();
    void machine();
    void messages();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet,
             bool shadow = false);
    void text(const char* s, float x, float y, float scale, int pal);
    bool startPressed() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Load load_[kLoads]{};
    Mote mote_[16]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool scooping_ = false;
    bool dumping_ = false;
    bool moving_ = false;
    bool carrying_ = false;
    int carried_ = 0;
    int dumped_ = 0;
    int focus_ = -1;
    const char* reason_ = "";
    float px_ = 0, py_ = 0;
    float face_ = 1;
    float step_ = 0;
    float t_ = 0;
    float shake_ = 0;
    float blip_ = 0;
    float tick_ = 0;
    float belt_ = 0;
    int clock_ = 0;
    uint32_t rng_ = 1;
};

}  // namespace qcler
