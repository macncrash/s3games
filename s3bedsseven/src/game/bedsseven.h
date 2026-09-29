// S3 BEDS SEVEN — a short row of beds. First gardener to seven.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace bedsseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BEDS SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }
    int marker() const;

private:
    enum class Mode { Title, Play, Win, Lose };

    struct Drop {
        float x, y, vy, life;
    };

    void begin();
    void play(float dt);
    void botAim(float& ax, bool& pour);
    void scoreBed(int i);
    void finish();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void put(const gs::Mipped& m, float x, float y, float w, float h, int pal, bool flip = false, bool shadow = false);
    void tone(int ch, float hz, float vol);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int you_ = 0;
    int them_ = 0;
    int face_ = 1;
    int pourBed_ = -1;
    float t_ = 0;
    float px_ = 74;
    float walk_ = 0;
    float fill_[BEDS] = {};
    float rivalFill_ = 0;
    float rivalWait_ = 0;
    float bloom_[BEDS] = {};
    float hold_ = 0;
    std::vector<Drop> drops_;
};

}  // namespace bedsseven
