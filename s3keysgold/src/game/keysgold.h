// S3 KEYS GOLD — a short keys. Only the gold counts double.
// Cream notes score nothing. Leave when a gold double is what crosses the line
// and the undoubled faces are still short of it.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace keysgold {

constexpr int kFace = 10;
constexpr int kLine = 150;
constexpr int kGoldsNeeded = 8;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KEYS GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool goldOut() const { return finisherGold_; }
    int score() const { return score_; }
    int bare() const { return bare_; }
    int golds() const { return golds_; }
    int cream() const { return cream_; }
    int hits() const { return hits_; }
    int line() const { return kLine; }

private:
    enum class Mode { Title, Play, Clear, Dead };

    struct Note {
        double time = 0;
        int lane = 0;
        int midi = 60;
        bool gold = false;
        bool done = false;
    };

    void startSong();
    void updatePlay();
    void miss();
    void win();
    void die();
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Image& img, float cx, float cy, float h, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool chartOk_ = true;
    bool finisherGold_ = false;
    bool leave_ = false;
    int score_ = 0;
    int bare_ = 0;
    int golds_ = 0;
    int cream_ = 0;
    int hits_ = 0;
    int misses_ = 0;
    int songFrame_ = 0;
    double t_ = 0;
    double clock_ = 0;
    float keyLit_[kLanes] = {};
    float beep_ = 0;
    Note notes_[16] = {};
    int noteN_ = 0;
};

}  // namespace keysgold
