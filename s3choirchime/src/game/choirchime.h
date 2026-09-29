// S3 CHOIRCHIME — three entries under the loft clock.
// They still have to land together. Landing before twelve does not end it.
// The chord that lands on the hour is the chime. Then the choir leaves.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace choirchime {

constexpr int kFpc = 6;
constexpr int kGraceSec = 30;
constexpr int kHourSec = 12 * 3600;
constexpr int kLeadSec = 90;
constexpr int kStartSec = kHourSec - kLeadSec;
constexpr int kSlop = 6;
constexpr int kTravel[3] = {48, 72, 96};
constexpr int kLandFrame = kLeadSec * kFpc;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CHOIRCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool onTheHour() const { return onHour(); }
    bool singing() const { return mode_ == Mode::Phrase && (cued_[0] || cued_[1] || cued_[2]); }
    int phrases() const { return phrases_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Phrase, Early, Chime, Leave, Fail, Over, Pause };

    void toTitle();
    void newGame();
    void beginPhrase();
    void cueVoice(int i);
    void resolve();
    void beginChime();
    void beginEarly();
    void beginFail(const char* why);
    bool audit() const;

    int secAt(int frames) const;
    int clockSec() const;
    bool onHourAt(int frames) const;
    bool onHour() const;
    bool pastHour() const;
    void split(int& h, int& m, int& s) const;
    bool possible() const;
    bool arrived() const;
    int spread() const;
    float progress(int i) const;
    bool hot(int i) const;

    void botCue();
    void strikeBell();
    void blip(int ch, float freq, float vol, float hold);
    void silenceVoices();
    void tickAudio(float dt);

    void draw();
    void backdrop();
    void board();
    void people();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void hudR(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, int pal, bool feet, int fog = 0);
    void ruleAt(const gs::Image& img, int x, int y, int w, int h, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Phrase;
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool clockOn_ = false;
    bool cued_[3] = {};
    int cueAt_[3] = {};
    int arr_[3] = {};
    int phrases_ = 0;
    int playFrames_ = 0;
    int titleFrames_ = 0;
    int showT_ = 0;
    int chimeFrames_ = 0;
    int leaveT_ = 0;
    int failFrames_ = 0;
    int strikes_ = 0;
    int anim_ = 0;
    int lastSpread_ = 0;
    float toneT_ = 0.f;
    float bellAmp_ = 0.2f;
};

}  // namespace choirchime
