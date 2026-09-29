// Hold the palisade door. Three minutes, or the gate is lost.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace palisade {

class Game : public gs::Cart {
public:
    const char* title() const override { return "PALISADE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int door() const { return door_; }
    int held() const { return held_; }

private:
    enum class Mode { Title, Fight, Victory, Lost, Pause };

    struct Foe {
        float x = 0;
        float vx = 0;
        int hp = 1;
        int flash = 0;
        int wind = 0;
        bool bash = false;
    };

    void begin();
    void update();
    void bot(float& mx, bool& thrust);
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void tone(float freq);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode heldMode_ = Mode::Fight;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int door_ = 100;
    int lives_ = 5;
    int held_ = 0;  // fight frames survived
    int face_ = 1;
    int thrust_ = 0;
    int thrustCd_ = 0;
    int hurt_ = 0;
    int spawnIn_ = 90;
    int spawnN_ = 0;
    int bashTick_ = 0;
    float px_ = 160;
    float beep_ = 0;
    std::vector<Foe> foes_;
};

}  // namespace palisade
