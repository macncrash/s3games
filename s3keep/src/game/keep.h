// S3 KEEP — hold the keep's door shut for three minutes.
#pragma once
#include <cstdint>

#include "console/system.h"
#include "game/art.h"

namespace keep {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KEEP"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    // 0 title, 1 the door, 2 held, 3 opened
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Won, Lost };
    static constexpr int CROW = 1;
    static constexpr int RAM = 2;
    static constexpr int HOLD = 180 * 60;

    struct Mark {
        int frame;
        int kind;
    };
    struct Spark {
        float x = 0, y = 0, vx = 0, vy = 0, life = 0;
    };
    struct Intent {
        bool shoulder = false;
        bool slam = false;
        bool brace = false;
        float lean = 0;
    };

    void begin();
    void schedule();
    void update();
    void hold(bool kept);
    Intent intent();
    void draw();
    void backdrop();
    void hud(int col, int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip = false);
    void blip(float freq);
    void burst(float x, float y);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool won_ = false;
    bool over_ = false;
    bool pushing_ = false;
    bool crowAnswered_ = false;
    bool botBraced_ = false;
    int age_ = 0;
    int score_ = 0;
    int slams_ = 0;
    int braced_ = 0;
    int gate_ = 0;
    int fanStep_ = -1;
    int markCount_ = 0;
    int markNext_ = 0;
    float gap_ = 0;
    float bar_ = 1;
    float stam_ = 1;
    float lean_ = 0;
    float crow_ = 0;
    float ram_ = 0;
    float braceCd_ = 0;
    float braceArm_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    float fanT_ = 0;
    float recoil_ = 0;
    Mark marks_[40]{};
    Spark sparks_[12]{};
};

}  // namespace keep
