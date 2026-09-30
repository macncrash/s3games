#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace sally {

// Dawn duel. The sally is yours. Fire on the third pace, or lose.
class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SALLY PACE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int pace() const { return pace_; }
    const char* reason() const { return reason_.c_str(); }

private:
    enum class Mode { Title, Duel, Over };

    void begin();
    void update();
    void draw();
    void sky();
    void ground();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void blip(int ch, float freq, float vol);
    void shot();
    void figure(float cx, int pal, bool flip, bool raised, bool smoked);
    bool wantFire() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int age_ = 0;
    int pace_ = 0;
    int call_ = 0;
    int walk_ = 0;
    int window_ = 0;
    bool resolved_ = false;
    bool youShot_ = false;
    bool foeShot_ = false;
    float youX_ = 132.f;
    float foeX_ = 188.f;
    float youFrom_ = 132.f;
    float foeFrom_ = 188.f;
    float youTo_ = 132.f;
    float foeTo_ = 188.f;
    int tone_ = 0;
    std::string reason_;
};

}  // namespace sally
