// S3 QUILLTAPE — a short quill. The drawer has to match the tape.
// HAIR, GOTH and SWASH drop into the drawer. RULE, BOLD and CURL pay the
// same and stay out. A close total is still open.
#pragma once

#include <vector>

#include "console/system.h"

namespace quilltape {

constexpr int kKinds = 6;
constexpr int kTapeN = 3;
constexpr int kMaxStrokes = 6;

struct Stroke {
    const char* name;
    int pay;
    int line;  // tape slot, or -1 if the stroke stays out of the drawer
};

inline constexpr Stroke kStroke[kKinds] = {
    {"HAIR", 1, 0}, {"RULE", 1, -1}, {"GOTH", 2, 1}, {"BOLD", 2, -1}, {"SWASH", 3, 2}, {"CURL", 3, -1},
};

inline int tapePay(int line) {
    for (int i = 0; i < kKinds; i++)
        if (kStroke[i].line == line) return kStroke[i].pay;
    return 0;
}

inline const char* tapeName(int line) {
    for (int i = 0; i < kKinds; i++)
        if (kStroke[i].line == line) return kStroke[i].name;
    return "";
}

inline int tapeSum() {
    int s = 0;
    for (int i = 0; i < kTapeN; i++) s += tapePay(i);
    return s;
}

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 QUILLTAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const { return held_[0] && held_[1] && held_[2]; }
    bool held(int i) const { return i >= 0 && i < kTapeN && held_[i]; }
    bool rules() const { return rules_; }
    bool writing() const { return mode_ == Mode::Write && cursor_ >= 3 && cursor_ <= 10; }
    int strokes() const { return strokes_; }
    int traps() const { return traps_; }
    int board() const { return board_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* reason() const { return reason_; }
    const char* say() const { return say_; }
    int phase() const;

private:
    enum class Mode { Title, Pick, Write, Leave, Lose };

    struct Pt {
        float x, y;
    };
    struct Dot {
        float x, y;
        int pal;
    };

    bool audit();
    void buildArt();
    void spr(const gs::Image& im, float x, float y, int pal, bool nib = false);
    void drawScene();
    void commit();
    void tally();
    void finish(bool win);

    gs::System* sys_ = nullptr;
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool held_[kTapeN] = {};
    int strokes_ = 0;
    int traps_ = 0;
    int board_ = 0;
    int well_ = 0;
    int ink_ = 0;
    int cursor_ = 0;
    int hold_ = 0;
    int wait_ = 0;
    float x_ = 48;
    float y_ = 150;
    const char* say_ = "SHORT QUILL";
    char reason_[64] = {};

    std::vector<Pt> path_;
    std::vector<Dot> laid_;

    gs::Image quill_{};
    gs::Image wellImg_{};
    gs::Image blot_{};
    gs::Image slip_{};
    gs::Image title_{};
    gs::Image hint_{};
    gs::Image start_{};
    gs::Image tapeWord_{};
    gs::Image drawerWord_{};
    gs::Image name_[kKinds]{};
    gs::Image done_{};
    gs::Image missed_{};
    gs::Image ends_{};
};

}  // namespace quilltape
