// S3 BEDS — six beds. Water them before the sun hits the wall.
#pragma once
#include <vector>

#include "art.h"
#include "console/system.h"

namespace beds {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BEDS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    // 0 title, 1 watering, 2 the beds held, 3 the sun hit the wall
    int marker() const;

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
    int wetCount() const;
    int nextDry() const;
    bool allWet() const;
    float sunU() const;
    float lightP() const;
    void win();
    void lose();
    void note(int i);
    void chirp();
    void draw();
    void sky(float p);
    void tintBrick(float p);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void put(const gs::Mipped& m, float x, float y, float w, float h, int pal, bool flip = false, bool shadow = false);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    float t_ = 0;
    float sunT_ = 0;
    float px_ = 28, py_ = 128;
    int face_ = 1;
    float walk_ = 0;
    float wet_[6] = {};
    float beep_ = 0;
    float tick_ = 0.4f;
    float bird_ = 1.2f;
    int birdN_ = 0;
    int melody_ = 0;
    bool pourHeard_ = false;
    float stuck_ = 0;
    float lastX_ = 0, lastY_ = 0;
    uint32_t rng_ = 0x0BED5u;
    std::vector<Bit> bits_;
};

}  // namespace beds
