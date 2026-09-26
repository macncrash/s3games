// S3 DEPOT RELIEF — one depot. Hold until the relief bell. Then it is done.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace depot {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DEPOT RELIEF"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int stopped() const { return stopped_; }
    int bays() const { return bays_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the watch, 2 the bell is ringing, 3 the watch has ended
    int marker() const;

private:
    enum class Mode { Title, Watch, Pause, Victory, Over };

    struct Spawn {
        float t;
        int kind;
        int track;
    };
    struct Foe {
        int kind = 0;
        int track = 0;
        int hp = 1;
        int points = 0;
        float x = 0;
        float age = 0;
        bool on = true;
    };
    struct Lamp {
        int track = 0;
        float x = 0;
        float prev = 0;
        bool on = true;
    };
    struct Puff {
        float x = 0, y = 0, t = 0, h = 16;
    };
    struct Pop {
        float x = 0, y = 0, t = 0;
        int pts = 0;
    };

    void beginWatch();
    void update(float dt);
    void draw();
    void tickAudio();
    int cover() const;
    int soonest(int track) const;
    void throwLamp();
    void brake();
    void kill(Foe& f);
    void breach(int track);
    void winWatch();
    void loseWatch();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);
    void shadow(float cx, float cy, float w);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    const gs::Mipped& foeImg(int kind, int frame) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool bell_ = false;
    bool stack_[3] = {true, true, true};
    const char* reason_ = "THE WATCH RAN OUT";
    int score_ = 0;
    int stopped_ = 0;
    int bays_ = 3;
    int track_ = 1;
    int spawnAt_ = 0;
    int fanStep_ = -1;
    float x_ = kStandX;
    float y_ = 151;
    float watch_ = 0;
    float modeT_ = 0;
    float lampCd_ = 0;
    float brakeCd_ = 0;
    float trackCd_ = 0;
    float shake_ = 0;
    float flash_ = 0;
    float beep_ = 0;
    float bellTick_ = 0;
    float fanT_ = 0;
    float moving_ = 0;
    std::vector<Spawn> script_;
    std::vector<Foe> foes_;
    std::vector<Lamp> lamps_;
    std::vector<Puff> puffs_;
    std::vector<Pop> pops_;
};

}  // namespace depot
