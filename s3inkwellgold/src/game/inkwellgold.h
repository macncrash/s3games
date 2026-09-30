// S3 INKWELL GOLD — one desk. Only the gold counts double.
// Cream ink scores its face and cannot buy the line.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace inkwellgold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 INKWELL GOLD"; }
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
    bool goldOut() const { return goldOut_; }
    // 0 title, 1 dipping, 2 won
    int marker() const;

    static constexpr int kLine = 8;

private:
    enum class Mode { Title, Play, Win, Lose };
    enum class Kind { Gold, Cream };

    void openDesk();
    void dip(Kind where);
    void pass();
    void advance();
    void judge();
    void paint();
    void blit(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void lineAt(int col, int row, const char* s, int pal);
    void lineC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool goldOut_ = false;
    int score_ = 0;
    int bare_ = 0;
    int golds_ = 0;
    int cream_ = 0;
    int faults_ = 0;
    int index_ = 0;
    int wait_ = 0;
    int life_ = 0;
    int hover_ = 0;  // 0 gold well, 1 pass, 2 cream well
    float dip_ = 0;
    Kind lamp_ = Kind::Gold;
    static constexpr int kDeck = 7;
    char deck_[kDeck + 1] = "GGCGCGG";
};

}  // namespace inkwellgold
