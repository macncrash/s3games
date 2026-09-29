// Solitaire. The bell rings only when the run is built, and only before the third try dies.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace solitairebell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SOLITAIRE BELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const { return rules_; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }
    int built() const { return built_; }
    const char* phase() const;
    const char* reason() const { return why_; }

    static constexpr int kCards = 8;
    static constexpr int kMaxDead = 3;

private:
    enum class Mode { Title, Play, Ring, Lose };

    void begin();
    void resetTry();
    void play(int i);
    void ring();
    void dieTry();
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
    bool rung_ = false;
    bool rules_ = false;
    int ranks_[kCards] = {};
    bool live_[kCards] = {};
    int next_ = 1;
    int built_ = 0;
    int cursor_ = 0;
    int dead_ = 0;
    int tryNo_ = 0;
    int wait_ = 0;
    int life_ = 0;
    float bellPh_ = 0;
    float bellAmp_ = 0.12f;
    const char* why_ = "";
};

}  // namespace solitairebell
