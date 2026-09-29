// S3 CHOIR SEVEN — three voices land together. First choir to seven.
#pragma once
#include "console/system.h"
#include "art.h"

namespace choirseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CHOIR SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }

private:
    enum class Mode { Title, Phrase, Hold, Win, Lose };

    void enterTitle();
    void beginMatch();
    void beginPhrase();
    void phraseFrame();
    void awardYou();
    void awardThem();
    void cueVoice(int i);
    int spread() const;
    bool possible() const;
    bool landed() const;
    float progress(int i) const;
    bool hot(int i) const;

    void draw();
    void backdrop();
    void board();
    void people();
    void scorePips();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, int pal, bool feet);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int you_ = 0;
    int them_ = 0;
    int tick_ = 0;
    int hold_ = 0;
    int anim_ = 0;
    int phrase_ = 0;
    bool cued_[3] = {};
    int cueAt_[3] = {};
    int arr_[3] = {};
    bool yours_ = true;
    char line_[32] = {};
};

}  // namespace choirseven
