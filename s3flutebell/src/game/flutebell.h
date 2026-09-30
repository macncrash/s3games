// S3 FLUTE BELL — play the flute.
// A brass bell hangs past the last mark. It rings only on a clean phrase.
// A wrong finger or a note that slips the mark: that try dies.
// The third dead try ends it. Leave only if the bell rang first.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace flutebell {

class Game : public gs::Cart {
public:
    static constexpr int kMarks = 4;

    const char* title() const override { return "S3 FLUTE BELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    int dead() const { return dead_; }
    int notes() const;
    const char* why() const { return why_; }

private:
    enum class Mode { Title, Play, Miss, Ring, Lose, Pause };

    void beginPhrase();
    void stepPlay();
    void blow();
    void drop();
    void ring();
    bool botBlow() const;
    float headX() const;
    void audio();
    void tone(float freq, float vol, int frames);
    void draw();
    void backdrop();
    void stage();
    void image(const gs::Mipped& m, float left, float top, float h, int pal, bool flip = false, int fog = 0);
    void spr(const gs::Mipped& m, float cx, float bottom, float h, int pal, bool flip = false, int fog = 0);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool did_[kMarks] = {};
    int finger_ = 0;
    int dead_ = 0;
    int hold_ = 0;
    int tick_ = 0;
    int missT_ = 0;
    int tone_ = 0;
    int fanT_ = 0;
    int breath_ = 0;
    int shake_ = 0;
    float t_ = 0;
    float phase_ = 0;
    float swing_ = 0;
    const char* why_ = "OPEN";
};

}  // namespace flutebell
