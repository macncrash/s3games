// S3 PARADE — reach the square. Don't get hit.
#pragma once
#include <string>
#include <vector>

#include "art.h"
#include "console/system.h"

namespace parade {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PARADE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int lives() const { return lives_; }
    int rows() const { return rows_; }

private:
    enum class Mode { Title, Play, Pause, Win, Over };

    struct Bit {
        float x, y, vx, vy, life;
    };
    struct Watcher {
        float x, y;
        int kind;
    };
    struct Spr {
        float key;
        int what;
        float x, y, h;
        int pal, frame, fog;
        bool flip;
    };

    void begin();
    void updatePlay();
    void botMove();
    void humanMove();
    void awardRows();
    void victory();
    void hurt();
    void music();
    void draw();
    void drawUi();
    void queueSpr(int what, float x, float y, float h, int pal, int frame, bool flip, int fog, float bias);
    void flushQueue();
    void spr(const gs::Mipped& m, float x, float y, float h, int pal, bool flip, int fog);
    void shade(float x, float y, float w);
    void textLine(const std::string& s, float x, float y, float h, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    bool confirm() const;
    bool overlaps(float x, float y, float t, float pad) const;
    bool pathClear(float x, float y0, float y1) const;
    bool inBand(float y) const;
    int nextLane() const;
    float landY(int idx) const;
    float yReach(int kind) const;
    void burst(float x, float y, int n);
    void tickBits();
    void countRows();
    float rnd();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool commit_ = false;
    bool moving_ = false;
    bool faceLeft_ = false;
    int lives_ = 5;
    int score_ = 0;
    int rows_ = 0;
    int scored_ = 0;
    int fan_ = -1;
    int lastStep_ = -1;
    int slide_ = 1;
    float px_ = 176.f, py_ = 760.f;
    float safeX_ = 176.f, safeY_ = 760.f;
    float t_ = 0;
    float inv_ = 0;
    float hurtFlash_ = 0;
    float shake_ = 0;
    float wait_ = 0;
    float commitX_ = 0, commitY_ = 0;
    float cam_ = 0;
    std::vector<Watcher> crowd_;
    std::vector<Watcher> flags_;
    std::vector<Bit> bits_;
    std::vector<Bit> ambient_;
    std::vector<Spr> queue_;
    uint32_t rng_ = 0x5A17u;
};

}  // namespace parade
