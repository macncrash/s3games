// S3 REDOUBT PURSUIT — you have the redoubt. Be the last machine still running.
#pragma once
#include <vector>

#include "art.h"
#include "console/system.h"

namespace redoubtpurs {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 REDOUBT PURSUIT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int boiler() const { return boiler_; }
    int stalled() const { return stalled_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the yard is live, 2 a machine just stopped, 3 the redoubt is decided
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };
    static constexpr int kFleet = 6;

    struct Rival {
        int lane = 1;
        int kind = 0;
        int hp = 2;
        float y = 96.f;
        int age = 0;
        bool running = true;
    };
    struct Bolt {
        int lane = 1;
        float y = 170.f;
        bool live = true;
    };

    void bootRoom();
    void begin();
    void update();
    void spawnDue();
    void fire();
    void stoke();
    void stopRival(Rival& r, bool hitWall);
    void winYard();
    void loseYard(const char* why);
    void botAct();
    void readPad();
    void road();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    float laneX(int lane) const;
    bool anyRunning() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int age_ = 0;
    int playF_ = 0;
    int lane_ = 1;
    int boiler_ = 8;
    int stalled_ = 0;
    int spawnIx_ = 0;
    int cool_ = 0;
    int stokeC_ = 0;
    int seize_ = 0;
    int hold_ = 0;
    int flash_ = 0;
    int blipT_ = 0;
    int score_ = 0;
    const char* reason_ = "";
    std::vector<Rival> rivals_;
    std::vector<Bolt> bolts_;
};

}  // namespace redoubtpurs
