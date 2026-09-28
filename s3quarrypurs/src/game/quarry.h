// S3 QUARRY PURSUIT — one quarry. Be the last machine still running.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace quarrypurs {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 QUARRY PURSUIT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int hull() const { return hull_; }
    int stopped() const { return stoppedCount(); }
    const char* reason() const { return reason_; }
    // 0 title, 1 the face is live, 2 a machine just stopped, 3 the quarry is decided
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };

    struct Machine {
        int kind = 0;
        int bench = 0;
        int dir = 1;
        int hp = 1;
        int maxHp = 1;
        float x = 0;
        float cool = 0;
        float flash = 0;
        float phase = 0;
        bool running = true;
    };
    struct Shot {
        int bench = 0;
        float x = 0;
        float prev = 0;
        float vx = 0;
        int from = -1;
    };
    struct Puff {
        int bench = 0;
        float x = 0;
        float y = 0;
        float vy = 0;
        float age = 0;
        float life = 0.4f;
        int kind = 0;
    };
    struct Plan {
        int bench = 1;
        int face = 1;
        int move = 0;
        bool fire = false;
    };

    void bootTitle();
    void begin();
    void resetWorld();
    void update(float dt);
    Plan botPlan() const;
    void launchPlayer();
    void launchBolt(int index);
    void hurtPlayer();
    void stopMachine(Machine& m);
    void winQuarry();
    void loseQuarry(const char* why);
    bool shotDanger(int bench) const;
    bool bodyClose(int bench) const;
    int safeBench() const;
    int quarry() const;
    int rivalsRunning() const;
    int stoppedCount() const;
    float halfOf(int kind) const;
    const char* nameOf(int kind) const;
    void serviceAudio();
    void draw();
    void face();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet, bool shadow);
    const gs::Mipped& bodyOf(int kind, int frame) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* reason_ = "NOT THE LAST";
    const char* banner_ = "";
    int hull_ = 8;
    int pBench_ = 0;
    int pDir_ = 1;
    float pX_ = 72.f;
    float t_ = 0;
    float watch_ = 0;
    float fireCd_ = 0;
    float benchCd_ = 0;
    float hurtT_ = 0;
    float shake_ = 0;
    float bannerT_ = 0;
    float blip_ = 0;
    int fanStep_ = -1;
    float fanT_ = 0;
    bool fanGood_ = true;
    int stopFlash_ = 0;
    std::vector<Machine> machines_;
    std::vector<Shot> shots_;
    std::vector<Puff> puffs_;
};

}  // namespace quarrypurs
