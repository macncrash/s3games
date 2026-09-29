// One viaduct. Make the magazine last longer than the raid.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace maga {

class Game : public gs::Cart {
public:
    const char* title() const override { return "VIADUCT MAGA"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int rounds() const { return rounds_; }
    int marker() const;  // 0 title, 1 raid, 2 ended

private:
    enum class Mode { Title, Raid, Over };

    struct Raider {
        int lane = 1;
        bool real = false;
        float z = 1;
        bool alive = true;
        bool peeled = false;
        int hit = 0;
    };

    void resetRaid();
    void updateRaid();
    void botAct();
    void tryFire();
    void drawWorld();
    void drawRaider(const Raider& r);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    bool anyReal() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool breached_ = false;
    int rounds_ = 7;
    int aim_ = 1;
    int flash_ = 0;
    int titleWait_ = 0;
    float t_ = 0;
    float cd_ = 0;
    float scroll_ = 0;
    int spawned_ = 0;
    int result_ = 0;  // 1 held, 2 spent, 3 deck
    static constexpr int kMag = 7;
    static constexpr float kRaid = 27.0f;
    static constexpr int kSpawns = 12;
    Raider men_[12]{};
};

}  // namespace maga
