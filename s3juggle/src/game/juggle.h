// S3 JUGGLE — three balls in the air for one minute.
#pragma once
#include <string>

#include "console/system.h"
#include "sprites.h"

namespace juggle {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 JUGGLE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int drops() const { return drops_; }
    int airFrames() const { return airFrames_; }

private:
    enum class Mode { Title, Count, Play, Drop, Over, Win, Pause };

    struct Flight {
        int ball = 0;
        int from = 0;
        int to = 0;
        int launch = 0;
        int land = 0;
        bool caught = false;
    };
    struct Fall {
        float x = 0, y = 0, vx = 0, vy = 0;
        int ball = 0;
        bool on = false;
    };
    struct Burst {
        float x = 0, y = 0;
        int life = 0;
        bool on = false;
    };

    void beginPattern();
    void updatePattern(bool scored);
    void updateCount();
    void updateDrop();
    void enterWin();
    void startDrop();
    void draw();
    void backdrop();
    void drawActors(float bob);
    void drawHud();
    void bodyCenter(float& x, float& y) const;
    void handPos(int hand, float& x, float& y) const;
    void arcPos(const Flight& f, float& x, float& y) const;
    void heldPos(int ball, float& x, float& y) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void panel();
    void wood();
    void melody();
    void blip(bool high);
    void ageTones();
    void burstAt(float x, float y);
    void ageBursts();
    void stepFan();
    void lights();
    int ballPal(int ball) const;
    void readTaps();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool fullCount_ = true;
    int drops_ = 0;
    int airFrames_ = 0;
    int pf_ = 0;
    int next_ = 0;
    int countF_ = 0;
    int dropF_ = 0;
    int anim_ = 0;
    int mel_ = 0;
    int toneLeft_[2] = {};
    int tapAt_[2] = {-100000, -100000};
    float stick_ = 0;
    int fanStep_ = -1;
    int fanWait_ = 0;
    bool live_[3] = {};
    Flight flight_[3]{};
    Fall fall_[3]{};
    Burst burst_[6]{};
};

}  // namespace juggle
