// S3 FLUTE CHIME — a short flute under a wall clock.
// Play the six notes clean. Nothing is won early.
// At twelve the hour either chimes or passes quiet.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace flutechime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FLUTE CHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool chimed() const { return chimed_; }
    int notes() const { return played_; }
    int hour() const { return hour_; }
    int minute() const { return minute_; }
    int second() const { return second_; }
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Play, Hold, Wait, Miss, Chime, Fail, Pause, Over };

    bool phraseDone() const;
    void begin();
    void blow(bool hit);
    void early();
    void strike();
    void passHour(const char* why);
    void readClock();
    float headX() const;
    void stepPlay();
    void tone(float freq, float vol, int frames);
    void draw();
    void backdrop();
    void image(const gs::Mipped& m, float left, float top, float h, int pal, bool flip = false);
    void spr(const gs::Mipped& m, float cx, float bottom, float h, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Play;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool chimed_ = false;
    bool clean_ = true;
    bool did_[kNotes] = {};
    int finger_ = 0;
    int played_ = 0;
    int playFrames_ = 0;
    int hold_ = 0;
    int tone_ = 0;
    int shake_ = 0;
    int breath_ = 0;
    int chimeStep_ = 0;
    int hour_ = 11;
    int minute_ = 59;
    int second_ = 50;
    float phase_ = 0;
    const char* why_ = "hour silent";
};

}  // namespace flutechime
