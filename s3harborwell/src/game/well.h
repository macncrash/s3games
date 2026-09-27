// S3 HARBOR WELL — keep the well standing through three waves.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>

namespace well {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HARBOR WELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int wave() const { return wave_; }
    int stones() const { return stones_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 a wave is on the quay, 2 the well is still standing between waves, 3 ended
    int marker() const;

private:
    enum class Mode { Title, Watch, Pause, Victory, Over };
    enum class Phase { Warn, Strike, Gap, Lull };

    void beginWatch();
    void startWave();
    void nextSurge();
    void resolveStrike();
    void update(float dt);
    void readBrace(int& dir, bool& any);
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0);
    void shadow(float cx, float cy, float w);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    int surgeDir() const;
    int surgeCount() const;
    float warnLen() const;
    float strikeLen() const;
    const char* braceName(int d) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Lull;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool braced_ = false;
    const char* reason_ = "";
    int score_ = 0;
    int wave_ = 0;
    int stones_ = 6;
    int surge_ = 0;
    int lean_ = 0;
    float phaseT_ = 0;
    float modeT_ = 0;
    float shake_ = 0;
    float swell_ = 0;
};

}  // namespace well
