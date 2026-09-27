// S3 HARBOR PURSUIT — one harbor. Be the last machine still running.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace harborpurs {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HARBOR PURSUIT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int hull() const { return hull_; }
    int stopped() const { return stoppedCount(); }
    const char* reason() const { return reason_; }
    // 0 title, 1 the fairway is live, 2 a machine just stopped, 3 the harbor is decided
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };

    struct Machine {
        int kind = 0;
        int lane = 0;
        int dir = 1;
        int hp = 1;
        float x = 0;
        float aim = 0;
        float cool = 0;
        float flash = 0;
        float wreck = 0;
        float wakeT = 0;
        float turn = 0;
        bool running = true;
    };
    struct Shot {
        int lane = 0;
        float x = 0;
        float prev = 0;
        float vx = 0;
        int from = -1;
    };
    struct Wake {
        int lane = 0;
        float x = 0;
        float y = 0;
        float age = 0;
        float life = 0.4f;
        int kind = 0;
    };
    struct Plan {
        int lane = 1;
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
    void winHarbor();
    void loseHarbor(const char* why);
    bool shotDanger(int lane) const;
    bool bodyClose(int lane) const;
    int safeLane() const;
    int quarry() const;
    int rivalsRunning() const;
    int stoppedCount() const;
    void serviceAudio();
    void draw();
    void sky();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);
    const gs::Mipped& bodyOf(int kind, int frame) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* reason_ = "NOT THE LAST";
    const char* banner_ = "";
    int hull_ = 6;
    int pLane_ = 1;
    int pDir_ = 1;
    float pX_ = 150.f;
    float t_ = 0;
    float watch_ = 0;
    float fireCd_ = 0;
    float laneCd_ = 0;
    float hurtT_ = 0;
    float shake_ = 0;
    float bannerT_ = 0;
    float blip_ = 0;
    float grace_ = 0;
    float tide_ = 0;
    int fanStep_ = -1;
    float fanT_ = 0;
    bool fanGood_ = true;
    std::vector<Machine> machines_;
    std::vector<Shot> shots_;
    std::vector<Wake> wakes_;
};

}  // namespace harborpurs
