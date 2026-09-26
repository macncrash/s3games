// S3 PINSTAPE — a short rack. The drawer has to match the tape.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace pinstape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PINSTAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    // 0 title, 1 aiming, 2 the ball is on the lane, 3 a line matched, 4 lost
    int phase() const;
    int lineNo() const { return line_; }
    const char* tapeLabel(int i) const;

private:
    enum class Mode { Title, Aim, Swing, Roll, Judge, Between, Win, Lose, Pause };
    enum class Verdict { None, Clear, Fail };

    struct Pin {
        bool down = false;
        float fall = 0;
        bool seen = false;
    };

    bool audit();
    void newGame();
    void resetRack();
    void release();
    void finishRoll();
    void resolve();
    void botInput(bool& action, bool& start, float& slide);
    void blip(float freq);
    void sink(float dt);
    int gotMask() const;
    int wantMask() const;
    int pointsOf(int mask) const;
    float meter() const;
    float liveX() const;
    int shotMask(float x) const;
    void maskText(int mask, char* out, int n, const char* none) const;
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool feet = false, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Verdict verdict_ = Verdict::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool layoutOk_ = false;
    int line_ = 0;
    int tries_ = 3;
    int balls_ = 1;
    int t_ = 0;
    float clock_ = 0;
    float aim_ = 0;
    float swingT_ = 0;
    float rollT_ = 0;
    float hold_ = 0;
    float beep_ = 0;
    float shake_ = 0;
    float ballX_ = 0;
    float ballZ_ = 0;
    char reason_[32] = {};
    Pin pin_[5]{};
};

}  // namespace pinstape
