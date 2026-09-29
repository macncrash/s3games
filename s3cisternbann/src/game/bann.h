// S3 CISTERN BANN — one cistern. Bring the banner back. Then it is done.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace cisternbann {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CISTERN BANN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool carrying() const { return has_; }
    float heroX() const { return px_; }
    float heroY() const { return py_; }
    // 0 title, 1 the descent, 2 banner in hand, 3 the climb, 4 ended
    int marker() const;

private:
    struct Plat {
        float x, y, w;
    };

    void begin();
    void bot(bool& left, bool& right, bool& jump);
    void stepPlay(bool left, bool right, bool jump);
    void win();
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);
    int onPlat() const { return plat_; }

    gs::System* sys_ = nullptr;
    Art art_{};
    Plat platBox_[5]{};
    int mode_ = 0;  // 0 title 1 play 2 victory
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool has_ = false;
    int face_ = 1;
    int plat_ = -1;
    int phase_ = 0;
    float px_ = 0, py_ = 0, vx_ = 0, vy_ = 0;
    float bannerX_ = 0, bannerY_ = 0;
    float eelX_ = 0, eelDir_ = 1;
    float t_ = 0, step_ = 0;
    int titleHold_ = 0;
};

}  // namespace cisternbann
