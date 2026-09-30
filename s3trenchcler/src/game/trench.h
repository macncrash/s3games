// S3 TRENCH CLER — one trench. Clear the ground before the clock dies.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace tcler {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TRENCH CLER"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int left() const { return kHeaps - cleared_; }
    const char* reason() const { return reason_; }
    int marker() const;

private:
    static constexpr int kHeaps = 6;
    enum class Mode { Title, Play, Pause, Won, Lost };

    struct Heap {
        float x = 0, z = 0;
        int kind = 0;
        float work = 0;
        bool gone = false;
    };
    struct Mote {
        float x = 0, z = 0, vx = 0, vz = 0, life = 0;
    };

    void resetTrench();
    void begin();
    void toTitle();
    void updatePlay();
    void botInput(float& ix, float& iz, bool& hold);
    void humanInput(float& ix, float& iz, bool& hold);
    void move(float ix, float iz);
    void updateWork(bool hold);
    int focusHeap() const;
    void win();
    void lose();
    void blip(float freq, float hold);
    void serviceAudio();
    void fadeMotes();
    void puff(float x, float z);
    float rnd();

    void draw();
    void trench();
    void heapsAndMan();
    void messages();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet,
             bool shadow = false);
    void text(const char* s, float x, float y, float scale, int pal);
    bool startPressed() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Heap heap_[kHeaps]{};
    Mote mote_[20]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool digging_ = false;
    bool moving_ = false;
    int focus_ = -1;
    int cleared_ = 0;
    const char* reason_ = "";
    float px_ = 0, pz_ = 0;
    float face_ = 1;
    float step_ = 0;
    float t_ = 0;
    float shake_ = 0;
    float blip_ = 0;
    float tick_ = 0;
    float swing_ = 0;
    int clock_ = 0;
    uint32_t rng_ = 1;
};

}  // namespace tcler
