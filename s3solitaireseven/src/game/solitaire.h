// S3 SOLITAIRE SEVEN — a short tableau. First to seven. Leave when that is true.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace solitaire {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SOLITAIRE SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int house() const { return house_; }
    // 0 title, 1 dealing, 2 won
    int marker() const;

    static constexpr int kSeven = 7;
    static constexpr int kCards = 10;

private:
    enum class Mode { Title, Play, Win, Lose };
    enum class Kind { You, House };

    struct Card {
        Kind kind = Kind::You;
        int rank = 1;
        bool live = true;
    };

    void deal();
    void fileYou(int i);
    void fileHouse(int i);
    void leave(int i);
    void fault();
    void judge();
    int nextYou() const;
    int firstHouse() const;
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
    int you_ = 0;
    int house_ = 0;
    int next_ = 1;
    int cursor_ = 0;
    int wait_ = 0;
    int life_ = 0;
    Card cards_[kCards]{};
};

}  // namespace solitaire
