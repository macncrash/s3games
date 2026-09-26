// S3 DEPOT PURSUIT — one depot. Be the last machine still running.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace depotpurs {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DEPOT PURSUIT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int hull() const { return hull_; }
    int stopped() const { return stoppedCount(); }
    const char* reason() const { return reason_; }
    // 0 title, 1 the yard is live, 2 a machine just stopped, 3 the depot is decided
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };

    struct Machine {
        int kind = 0;
        int track = 0;
        int dir = 1;
        int hp = 1;
        int maxHp = 1;
        float x = 0;
        float aim = 0;
        float cool = 0;
        float hop = 0;
        float flash = 0;
        float wreck = 0;
        float puffT = 0;
        float phase = 0;
        float turn = 0;
        bool running = true;
    };
    struct Shot {
        int track = 0;
        float x = 0;
        float prev = 0;
        float vx = 0;
        int from = -1;
    };
    struct Puff {
        int track = 0;
        float x = 0;
        float y = 0;
        float vy = 0;
        float age = 0;
        float life = 0.45f;
        int kind = 0;
    };
    struct Plan {
        int track = 1;
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
    void winYard();
    void loseYard(const char* why);
    bool shotDanger(int track) const;
    bool bodyClose(int track) const;
    int safeTrack() const;
    int quarry() const;
    int onTrack(int track) const;
    int rivalsRunning() const;
    int stoppedCount() const;
    void serviceAudio();
    void draw();
    void sky();
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
    int pTrack_ = 1;
    int pDir_ = 1;
    float pX_ = 140.f;
    float t_ = 0;
    float watch_ = 0;
    float fireCd_ = 0;
    float trackCd_ = 0;
    float hurtT_ = 0;
    float shake_ = 0;
    float bannerT_ = 0;
    float blip_ = 0;
    float grace_ = 0;
    float puffT_ = 0.1f;
    int fanStep_ = -1;
    float fanT_ = 0;
    bool fanGood_ = true;
    std::vector<Machine> machines_;
    std::vector<Shot> shots_;
    std::vector<Puff> puffs_;
};

}  // namespace depotpurs
