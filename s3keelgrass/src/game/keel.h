// S3 KEELGRASS — put the keel on the grass and come to a full stop.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace keel {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KEELGRASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float speed() const { return vx_; }

private:
    enum class Mode { Title, Sail, Win, Fail };

    void resetRun();
    void physics();
    void draw();
    void sky();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void bot(bool& thrustF, bool& thrustB, bool& keelDn, bool& keelUp, bool& start);
    bool onGrass() const;
    bool inWater() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    float x_ = 0;
    float vx_ = 0;
    float keel_ = 0.15f;
    float bob_ = 0;
    float t_ = 0;
    float stillG_ = 0;
    float stillW_ = 0;
    float beep_ = 0;
    int hold_ = 0;
    bool touched_ = false;
    bool dug_ = false;
    std::string why_;
};

}  // namespace keel
