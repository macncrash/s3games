// S3 BUNKER PURSUIT — you have the bunker. Be the last machine still running.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace bunkerpurs {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BUNKER PURSUIT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int stalled() const { return stalled_; }
    int fleet() const { return kFleet; }
    int boiler() const { return boiler_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the bunker is live, 2 a machine just stopped, 3 the bunker is decided
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };
    static constexpr int kFleet = 6;
    static constexpr int kBays = 3;

    struct Rival {
        int bay = 1;
        int kind = 0;
        int hp = 1;
        float x = 300;
        float cool = 0.8f;
        bool running = true;
    };
    struct Bolt {
        int bay = 0;
        float x = 0;
        float vx = 0;
        bool player = false;
        bool live = true;
    };
    struct Mote {
        float x = 0, y = 0, vx = 0, vy = 0, life = 0;
        int kind = 0;
    };

    void bootRoom();
    void begin();
    void update();
    void spawnDue();
    void firePlayer();
    void fireRival(Rival& r);
    void stopRival(Rival& r);
    void hurtBoiler();
    void winBunker();
    void loseBunker(const char* why);
    void botAct();
    void readPad();
    void puff(float x, float y, int n, int kind);
    void tickMotes();
    void blip(int ch, float freq, float vol);
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip);
    void text(const char* s, float x, float y, int pal);
    float bayY(int bay) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int age_ = 0;
    int playF_ = 0;
    int bay_ = 1;
    int boiler_ = 8;
    int stalled_ = 0;
    int spawnIx_ = 0;
    int cool_ = 0;
    int seize_ = 0;
    int hold_ = 0;
    int blipT_ = 0;
    int flash_ = 0;
    const char* reason_ = "";
    std::vector<Rival> rivals_;
    std::vector<Bolt> bolts_;
    Mote motes_[28] = {};
};

}  // namespace bunkerpurs
