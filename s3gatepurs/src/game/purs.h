// S3 GATE PURSUIT — at the gate, be the last machine still running.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace purs {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GATE PURSUIT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int hull() const { return hull_; }
    int stopped() const { return stoppedCount(); }
    const char* reason() const { return reason_; }
    // 0 title, 1 the pursuit, 2 a machine just stopped, 3 the watch has ended
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };

    struct Machine {
        int kind = 0;
        int lane = 0;
        int dir = 1;
        int hp = 1;
        int maxHp = 1;
        float lat = 0;
        float z = 10;
        float hold = 20;
        float wind = 0;
        float cool = 0;
        float flash = 0;
        float wreck = 0;
        float phase = 0;
        bool running = true;
    };
    struct Shot {
        float lat = 0;
        float z = 0;
        float prev = 0;
        int from = -1;
    };
    struct Puff {
        float lat = 0;
        float z = 0;
        float age = 0;
        float life = 0.4f;
        int kind = 0;
    };
    struct Prop {
        float lat = 0;
        float z = 0;
        int kind = 0;
    };
    struct Proj {
        float x = 0, y = 0, ppm = 0;
        bool ok = false;
    };

    void bootTitle();
    void begin();
    void resetWorld();
    void update(float dt);
    void botGoal(float& goal, bool& fire);
    void launchPlayer();
    void launchBolt(int index);
    void hurtPlayer();
    void stopMachine(Machine& m);
    void winWatch();
    void loseWatch(const char* why);
    void serviceAudio();
    void draw();
    void road(float shx);
    void drawGate(float shx);
    void drawMachine(const Machine& m, float shx, bool you);
    Proj project(float lat, float z) const;
    int fogFor(float z) const;
    bool laneHot(float lat) const;
    int stoppedCount() const;
    int rivalsRunning() const;
    const Machine* quarry() const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* reason_ = "WATCH OVER";
    const char* banner_ = "";
    int hull_ = 6;
    float t_ = 0;
    float watch_ = 0;
    float px_ = 0;
    float fireCd_ = 0;
    float hurtT_ = 0;
    float shake_ = 0;
    float scroll_ = 0;
    float travel_ = 0;
    float grace_ = 0;
    float bannerT_ = 0;
    float blip_ = 0;
    int fanStep_ = -1;
    float fanT_ = 0;
    bool fanGood_ = true;
    std::vector<Machine> machines_;
    std::vector<Shot> shots_;
    std::vector<Puff> puffs_;
    std::vector<Prop> props_;
};

}  // namespace purs
