// S3 FLUTE GOLD — a short flute. Only the gold counts double.
// Cream notes keep their face. The bare sum stays under the line.
// Leave on the last gold, when the double is the only thing that clears it.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace flutegold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FLUTE GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finisherGold() const { return finisherGold_; }
    int score() const { return score_; }
    int bare() const { return bare_; }
    int golds() const { return golds_; }
    int cream() const { return cream_; }
    int notes() const { return played_; }
    int line() const { return kLine; }

private:
    enum class Mode { Title, Play, Hold, Ready, Miss, Pause, Over };

    bool open() const;
    void begin();
    void blow(bool hit);
    void leave();
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
    bool finisherGold_ = false;
    bool spoiled_ = false;
    bool did_[kNotes] = {};
    int finger_ = 0;
    int played_ = 0;
    int score_ = 0;
    int bare_ = 0;
    int golds_ = 0;
    int cream_ = 0;
    int hold_ = 0;
    int tone_ = 0;
    int shake_ = 0;
    int breath_ = 0;
    float phase_ = 0;
    const char* why_ = "";
};

}  // namespace flutegold
