#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace glider {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GLIDER"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int lives() const { return lives_; }
    const char* result() const { return result_[0] ? result_ : "still airborne"; }
    float flightX() const { return x_; }
    float flightAgl() const { return agl_; }
    float flightSpd() const { return v_; }
    float flightVar() const { return vy_; }

private:
    enum class Mode { Title, Fly, Pause, Dead, Over, Victory };

    struct Mark {
        float x, y, h;
        int kind;
    };

    void newGame();
    void launch();
    void steer(float& nose, float& spoil);
    void fly(float nose, float spoil);
    void succeed();
    void fail(const char* why);
    void draw(float camX, float camY, float pitch, bool craft);
    void sky();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0, bool feet = false);
    void sprBox(const gs::Mipped& m, float cx, float top, float w, float h, int pal);
    void blip(bool high);
    void layoutWorld();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int score_ = 0;
    int lives_ = 3;
    char result_[48] = {};
    float x_ = 0, y_ = 0, v_ = 22, pitch_ = 0, vy_ = 0, wind_ = 0, agl_ = 0;
    float spoil_ = 0;
    float t_ = 0, deadT_ = 0, fanT_ = 0, beep_ = 0, varioT_ = 0, shake_ = 0;
    int fanStep_ = -1;
    bool spoilWas_ = false;

    std::vector<Mark> trees_, clouds_, birds_;
    float sockX_ = 0, barnX_ = 0;
};

}  // namespace glider
