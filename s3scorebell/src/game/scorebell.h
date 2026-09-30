// S3 SCORE BELL — play the written score.
// A brass bell hangs past the last bar. It rings only on a clean phrase.
// A miss, a wrong lane, or a note that slips the bar: that try dies.
// The third dead try ends it. Leave only if the bell rang first.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace scorebell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SCORE BELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    int dead() const { return dead_; }
    int struck() const { return struck_; }
    const char* why() const { return why_; }

private:
    enum class Mode { Title, Play, Ring, Leave, Lose };

    struct Note {
        int lane = 0;
        float beat = 0;
        int state = 0;  // 0 waiting, 1 struck, 2 missed
    };

    void freshPhrase();
    void begin();
    void strike(Note& n);
    void dieTry(const char* why);
    void ring();
    void leave();
    int lanePressed() const;
    float noteX(const Note& n) const;
    void blip(float freq);
    void audio();
    void backdrop();
    void hud(int col, int row, const char* s);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void draw();
    void update(float dt);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Note notes_[6]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    int dead_ = 0;
    int struck_ = 0;
    int age_ = 0;
    int chime_ = -1;
    int chimeWait_ = 0;
    float song_ = 0;
    float t_ = 0;
    float swing_ = 0;
    const char* why_ = "OPEN";

    static constexpr int kNotes = 6;
    static constexpr float kLine = 92.f;
    static constexpr float kSpeed = 84.f;
};

}  // namespace scorebell
