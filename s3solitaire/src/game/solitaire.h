// S3 SOLITAIRE — one deal. Clear the tableau.
// Aces through sevens, all face up. Build down red on black. Send them home in suit.
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace sol {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SOLITAIRE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int moves() const { return moves_; }
    int home() const { return home_; }

private:
    enum class Mode { Title, Play, Pause, Stuck, Victory };

    struct Move {
        int kind = 0;  // 0 foundation, 1 tableau
        int src = 0;
        int at = 0;
        int dst = -1;
    };
    struct Table {
        std::array<std::vector<uint8_t>, 7> col;
        std::array<int8_t, 4> found{{-1, -1, -1, -1}};
    };
    struct Flight {
        uint8_t id = 0;
        float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    };

    void startDeal();
    void layout(Table& t) const;
    std::vector<Move> solve(const Table& t) const;
    bool legal(const Table& t, Move m) const;
    void apply(Table& t, Move m) const;
    bool commit(Move m);
    bool anyMove(const Table& t) const;
    int homeOf(const Table& t) const;
    bool cleared(const Table& t) const;
    void onLand();
    void playHuman(const gs::Pad& pad);
    void blip(int kind);
    void fanfare();
    void fanTick();

    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float x, float y, float w, float h, int pal, bool shadow = false);
    void cardAt(uint8_t id, float x, float y);
    int cascade(int n) const;
    void cell(int col, int index, int n, float& x, float& y) const;
    void foundAt(int suit, float& x, float& y) const;
    bool inFlight(uint8_t id) const;
    std::string hintLine(Move m) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Table table_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int moves_ = 0;
    int home_ = 0;
    int cursor_ = 4;
    int hold_ = -1;
    int holdAt_ = 0;
    int intro_ = 0;
    int gap_ = 0;
    int celebrate_ = 0;
    int hint_ = 0;
    float flyT_ = 1;
    float beep_ = 0;
    float fanT_ = 0;
    float t_ = 0;
    int fanStep_ = -1;
    Move hintMv_{};
    bool hintOn_ = false;
    std::vector<Move> script_;
    size_t scriptAt_ = 0;
    std::vector<Flight> fly_;
    std::vector<Table> undo_;
};

}  // namespace sol
