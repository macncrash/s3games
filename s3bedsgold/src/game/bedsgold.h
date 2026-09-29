// S3 BEDS GOLD — six beds. Gold blooms count two. Cream counts one.
// A cream bed that would reach the line does not count.
// Only a gold bed can finish, and the undoubled beds stay under the line.
#pragma once
#include <vector>

#include "art.h"
#include "console/system.h"

namespace bedsgold {

constexpr int kLine = 6;
constexpr int kBeds = 6;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BEDS GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finisherGold() const { return finisherGold_; }
    int gold() const { return gold_; }
    int cream() const { return cream_; }
    int score() const { return score_; }
    int beds() const { return watered_; }
    int line() const { return kLine; }
    int bare() const { return gold_ + cream_; }
    const char* say() const { return say_; }

private:
    enum class Mode { Title, Play, Win, Lose };

    struct Bit {
        float x, y, vx, vy, life;
        int kind;
    };

    void startRound();
    void move(float ax, float ay, float dt);
    bool blocked(float x, float y) const;
    int atBed() const;
    void resolve(int bed);
    void win();
    void lose();
    bool paid() const;
    void put(const gs::Mipped& m, float x, float y, float w, float h, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finisherGold_ = false;
    bool pourHeard_ = false;
    int gold_ = 0;
    int cream_ = 0;
    int score_ = 0;
    int watered_ = 0;
    int refused_ = 0;
    float t_ = 0;
    float sunT_ = 0;
    float px_ = 40, py_ = 128;
    int face_ = 1;
    float walk_ = 0;
    float wet_[kBeds] = {};
    int took_[kBeds] = {};  // 0 open, 1 counted, 2 refused
    const char* say_ = "";
    std::vector<Bit> bits_;
    uint32_t rng_ = 0xB3D5u;
};

}  // namespace bedsgold
