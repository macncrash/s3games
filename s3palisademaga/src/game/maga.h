#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace palisade {

class Game : public gs::Cart {
public:
    const char* title() const override { return "PALISADE MAGA"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int rounds() const { return rounds_; }
    int wall() const { return wall_; }
    // 0 title, 1 the raid, 2 magazine held, 3 the raid won the night
    int marker() const;

private:
    enum class Mode { Title, Raid, Won, Lost };

    struct Order {
        float t;
        float x;
        int kind;  // 0 presses the stakes, 1 turns back
    };
    struct Foe {
        float x, y, flash;
        int kind;
        int phase;
        float speed;
        bool live;
    };
    struct Slug {
        float x, y;
    };

    void beginRaid();
    void update(float dt);
    void draw();
    void hudText(int col, int row, const char* s);
    void spr(const gs::Image& img, float x, float y, int w, int h, int pal, bool flip = false);
    void shoot();
    void stick(float& dir, bool& fire);
    void finish(bool hold);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int score_ = 0;
    int rounds_ = 0;
    int wall_ = 0;
    int kills_ = 0;
    float t_ = 0;
    float raid_ = 0;
    float px_ = 160;
    float face_ = 1;
    float cool_ = 0;
    float shake_ = 0;
    int spawnIx_ = 0;
    int fail_ = 0;  // 1 magazine, 2 palisade
    std::vector<Order> script_;
    std::vector<Foe> foes_;
    std::vector<Slug> slugs_;
};

}  // namespace palisade
