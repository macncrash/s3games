// S3 TILE GOLD — set the floor. Only the gold counts double.
// Cream scores its face and cannot buy the line.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace tilegold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TILE GOLD"; }
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
    // 0 title, 1 tiling, 2 won
    int marker() const;

    static constexpr int kLine = 8;

private:
    enum class Mode { Title, Play, Win, Lose };
    enum class Kind { Gold, Cream };

    void openNight();
    void setTile(Kind where);
    void leave();
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
    int hover_ = 0;  // 0 gold bed, 1 leave, 2 cream bed
    Kind hand_ = Kind::Gold;
    static constexpr int kStack = 7;
    char stack_[kStack + 1] = "GGCGCGG";
    char laid_[kStack] = {};
};

}  // namespace tilegold
