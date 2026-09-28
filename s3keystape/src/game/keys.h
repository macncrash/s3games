// S3 KEYSTAPE — file the keys until the drawer matches the tape, then leave.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace keys {

constexpr int NKEYS = 5;
constexpr int NWARDS = 3;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KEYSTAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int cut(int key, int ward) const;
    int tapeCut(int key, int ward) const;

private:
    enum class Mode { Title, Play, Leave, Victory };

    void deal();
    bool matched() const;
    void raise(int dir);
    void tryLeave();
    void botAct();
    void human();
    void blip(float freq);
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float x, float y, float h, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int have_[NKEYS][NWARDS] = {};
    int tape_[NKEYS][NWARDS] = {};
    int sel_ = 0;
    int ward_ = 0;
    int t_ = 0;
    int shake_ = 0;
    int cool_ = 0;
    int leaveT_ = 0;
    int door_ = 0;
};

}  // namespace keys
