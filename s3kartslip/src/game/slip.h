// S3 KARTSLIP — take the kart and berth in the slip before the tide turns.
// The other crew is the clock.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace slip {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KARTSLIP"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float tideLeft() const { return tide_; }
    float crewBehind() const { return z_ - rz_; }
    // 0 title, 1 racing, 2 berthed, 3 lost
    int marker() const;

private:
    enum class Mode { Title, Race, Win, Lose };

    struct Prop {
        float z, x, h;
        int kind;  // 0 crate, 1 post, 2 boat, 3 buoy
    };

    void startRun();
    void update(float dt);
    void draw();
    void sky();
    void quay();
    void actors();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0, bool shadow = false);
    void project(float wz, float wx, float worldH, float& sx, float& sy, float& sh, int& fog) const;
    void blip(float freq, float vol);
    void engine();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool crewIn_ = false;
    int hold_ = 0;
    float t_ = 0;
    float tide_ = 0;
    float z_ = 0, x_ = 0, spd_ = 0;
    float rz_ = 0, rx_ = 0;
    float lean_ = 0;
    float bump_ = 0;
    float splash_ = 0;
    const char* cause_ = "";
    Prop props_[24] = {};
    int nprop_ = 0;
};

}  // namespace slip
