// A short solitaire. Build the run, then wait. The hour has to chime at 12:00:00.
// A finished run before the hour is still short. A wrong card leaves the hour silent.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace solitairechime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SOLITAIRE CHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool chimed() const { return chimed_; }
    bool rules() const { return rules_; }
    int built() const { return built_; }
    int hour() const { return frozen_ ? fh_ : liveHour(); }
    int minute() const { return frozen_ ? fm_ : liveMinute(); }
    int second() const { return frozen_ ? fs_ : liveSecond(); }
    const char* phase() const;
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Play, Wait, Chime, Fail };

    void begin();
    void play(int i);
    void strikeHour();
    void passHour(const char* why);
    void splitLive(int& h, int& m, int& s) const;
    int liveHour() const;
    int liveMinute() const;
    int liveSecond() const;
    void paint();
    void blit(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void lineAt(int col, int row, const char* s, int pal);
    void lineC(int row, const char* s, int pal);
    int indexOf(int rank) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool chimed_ = false;
    bool spoiled_ = false;
    bool frozen_ = false;
    bool rules_ = false;
    int ranks_[kCards] = {};
    bool live_[kCards] = {};
    int next_ = 1;
    int built_ = 0;
    int cursor_ = 0;
    int playFrames_ = 0;
    int life_ = 0;
    int hold_ = 0;
    int fh_ = 12, fm_ = 0, fs_ = 0;
    float bellPh_ = 0;
    float bellAmp_ = 0.1f;
    const char* why_ = "hour silent";
};

}  // namespace solitairechime
