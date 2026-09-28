// S3 MILL PURSUIT — you have the mill. Be the last machine still running.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace millp {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MILL PURSUIT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int stalled() const { return stalled_; }
    int fleet() const { return kFleet; }
    int mill() const { return millHp_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the yard is live, 2 a machine just stopped, 3 the mill is decided
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };
    static constexpr int kFleet = 6;

    struct Rival {
        int lane = 1;
        int kind = 0;
        int hp = 1;
        float x = 330.f;
        float cool = 0.9f;
        bool running = true;
    };
    struct Bolt {
        int lane = 0;
        float x = 0;
        float vx = 0;
        bool player = false;
        bool live = true;
    };
    struct Mote {
        float x = 0, y = 0, vx = 0, vy = 0, life = 0;
    };

    void bootYard();
    void begin();
    void update();
    void spawnDue();
    void firePlayer();
    void fireRival(Rival& r);
    void stopRival(Rival& r);
    void hurtMill();
    void winMill();
    void loseMill(const char* why);
    void botAct();
    void puff(float x, float y, int n);
    void tickMotes();
    void blip(int ch, float freq, float vol);
    void serviceAudio();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    float laneY(int lane) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int age_ = 0;
    int playF_ = 0;
    int lane_ = 1;
    int millHp_ = 9;
    int stalled_ = 0;
    int spawnIx_ = 0;
    int cool_ = 0;
    int seize_ = 0;
    int sail_ = 0;
    float hold_ = 0;
    float blip_ = 0;
    const char* reason_ = "";
    std::vector<Rival> rivals_;
    std::vector<Bolt> bolts_;
    Mote motes_[24] = {};
};

}  // namespace millp
