// S3 BEDSMARK — six beds. One holds the mark.
// Water that bed and the mark opens. Lift the stake.
// That finished mark ends the cartridge. The other beds are not the job.
#pragma once
#include <vector>

#include "art.h"
#include "console/system.h"

namespace bedsmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BEDSMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }
    bool lifted() const { return lifted_; }
    bool opened() const { return opened_; }
    int markBed() const { return mark_; }
    float wetMark() const { return wet_[mark_]; }

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
    void lift();
    void lose();
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
    bool opened_ = false;
    bool lifted_ = false;
    bool finished_ = false;
    bool pourHeard_ = false;
    float t_ = 0;
    float sunT_ = 0;
    float px_ = 40, py_ = 128;
    int face_ = 1;
    float walk_ = 0;
    float wet_[6] = {};
    int mark_ = 2;
    std::vector<Bit> bits_;
    uint32_t rng_ = 0xB3D5u;
};

}  // namespace bedsmark
