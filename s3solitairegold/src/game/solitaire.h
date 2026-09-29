// S3 SOLITAIRE GOLD — a short tableau. Only the gold counts double.
// Cream scores its face and cannot buy the line.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace solitaire {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SOLITAIRE GOLD"; }
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
    // 0 title, 1 dealing, 2 won
    int marker() const;

    static constexpr int kLine = 8;
    static constexpr int kCards = 6;

private:
    enum class Mode { Title, Play, Win, Lose };
    enum class Kind { Gold, Cream };

    struct Card {
        Kind kind = Kind::Gold;
        int rank = 1;
        bool live = true;
    };

    void deal();
    void fileGold(int i);
    void fileCream(int i);
    void leave(int i);
    void fault();
    void judge();
    int liveCount() const;
    int nextGold() const;
    int firstCream() const;
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
    int next_ = 1;
    int cursor_ = 0;
    int wait_ = 0;
    int life_ = 0;
    Card cards_[kCards]{};
};

}  // namespace solitaire
