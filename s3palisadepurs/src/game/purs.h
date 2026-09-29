// S3 PALISADE PURSUIT — one palisade. Be the last machine still running.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace palisadepurs {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PALISADE PURSUIT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int fire() const { return fire_; }
    int stopped() const { return stopped_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the palisade is live, 2 a machine just stopped, 3 the wall is decided
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };
    static constexpr int kFleet = 5;
    static constexpr int kGaps = 3;

    struct Rival {
        int gap = 1;
        int kind = 0;
        int hp = 1;
        float x = 300;
        float cool = 1.2f;
        bool running = true;
    };
    struct Bolt {
        int gap = 0;
        float x = 0;
        float vx = 0;
        bool player = false;
        bool live = true;
    };
    struct Mote {
        float x = 0, y = 0, vx = 0, vy = 0, life = 0;
    };

    void bootField();
    void begin();
    void update();
    void spawnDue();
    void firePlayer();
    void fireRival(Rival& r);
    void stopRival(Rival& r);
    void hurtEngine();
    void winWall();
    void loseWall(const char* why);
    void botAct();
    void readPad();
    void puff(float x, float y, int n);
    void tickMotes();
    void blip(int ch, float freq, float vol);
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip);
    void text(const char* s, float x, float y, int pal);
    float gapY(int gap) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int age_ = 0;
    int playF_ = 0;
    int gap_ = 1;
    int fire_ = 6;
    int stopped_ = 0;
    int spawnIx_ = 0;
    int cool_ = 0;
    int seize_ = 0;
    int hold_ = 0;
    int blipT_ = 0;
    int flash_ = 0;
    const char* reason_ = "";
    std::vector<Rival> rivals_;
    std::vector<Bolt> bolts_;
    Mote motes_[24] = {};
};

}  // namespace palisadepurs
