// S3 CISTERN POUC — carry the pouch across the cistern. Miss it and the watch is over.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace cisternpouc {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CISTERN POUC"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool carrying() const { return held_; }
    float heroX() const { return px_; }
    float heroY() const { return py_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 in the cistern, 2 pouch in hand, 3 crossing up, 4 ended
    int marker() const;

private:
    struct Plat {
        float x, y, w;
    };

    void begin();
    void win();
    void lose(const char* why);
    void bot(bool& left, bool& right, bool& jump);
    void stepPlay(bool left, bool right, bool jump);
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);

    gs::System* sys_ = nullptr;
    Art art_{};
    Plat platBox_[4]{};
    int mode_ = 0;  // 0 title 1 play 2 victory 3 loss
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool held_ = false;
    int face_ = 1;
    int plat_ = -1;
    int phase_ = 0;
    int watch_ = 0;
    int titleHold_ = 0;
    const char* reason_ = "";
    float px_ = 0, py_ = 0, vx_ = 0, vy_ = 0;
    float pouchX_ = 0, pouchY_ = 0;
    float bobY_ = 0, bobV_ = 1;
    float t_ = 0, step_ = 0;
};

}  // namespace cisternpouc
