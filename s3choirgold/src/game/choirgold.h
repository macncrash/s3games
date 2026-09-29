// S3 CHOIR GOLD — three entries. Only the gold counts double.
#pragma once
#include "console/system.h"
#include "game/pictures.h"

namespace choirgold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CHOIR GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int bare() const { return bare_; }
    int golds() const { return golds_; }
    int cream() const { return cream_; }
    int line() const { return kLine; }
    int phrase() const { return phrase_; }
    bool goldOut() const { return goldOut_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 phrase, 2 landing, 3 stopped, 4 left on a gold double
    int marker() const;

    static constexpr int kLine = 12;

private:
    enum class Mode { Title, Phrase, Pause, Resolve, Stop, Victory };

    void enterTitle();
    void startAnthem();
    void beginPhrase(int p);
    void phraseFrame();
    void cueVoice(int i);
    void succeed();
    void fail(const char* why);
    void judge();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void blit(const gs::Image& img, float cx, float cy, float w, float h, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool goldOut_ = false;
    int phrase_ = 0;
    int tick_ = 0;
    int hold_ = 0;
    int anim_ = 0;
    int score_ = 0;
    int bare_ = 0;
    int golds_ = 0;
    int cream_ = 0;
    bool cued_[3] = {};
    bool takenCream_[3] = {};
    int arr_[3] = {};
    char reason_[40] = {};
};

}  // namespace choirgold
