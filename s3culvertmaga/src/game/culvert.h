// S3 CULVERT MAGA — one magazine at the pipe mouth.
// A round has to still be seated when the raid clock runs out.
// Firing the last round, or letting a lamp reach the grate, ends the watch.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace cmaga {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CULVERT MAGA"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int rounds() const { return rounds_; }
    int stopped() const { return stopped_; }
    float raidTime() const { return raidT_; }
    const char* result() const;
    // 0 title, 1 the watch, 2 the raid has ended
    int marker() const;

private:
    enum class Mode { Title, Watch, Pause, Won, Lost };
    enum class Fail { None, Spent, Grate };
    enum class Kind { Lamp, Shade };
    enum class Phase { Walk, Turn, Down };

    struct Foe {
        Kind kind = Kind::Lamp;
        Phase phase = Phase::Walk;
        float z = 1.f;
        float x = 0;
        float age = 0;
        bool on = true;
    };

    void beginWatch();
    void updateWatch(float dt);
    void botAct();
    void humanAct();
    void squeeze();
    void lose(Fail why);
    void win();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);
    void text(const std::string& s, float x, float y, float scale, int pal);
    Foe* inWindow();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Fail fail_ = Fail::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int rounds_ = 0;
    int stopped_ = 0;
    int next_ = 0;
    float aim_ = 0;
    float t_ = 0;
    float raidT_ = 0;
    float cool_ = 0;
    float flash_ = 0;
    float shake_ = 0;
    std::vector<Foe> foes_;
};

}  // namespace cmaga
