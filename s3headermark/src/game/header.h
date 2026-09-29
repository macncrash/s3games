// S3 HEADERMARK — the header has one job: set down on the mark.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace headermark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HEADERMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return raceT_; }
    // 0 title, 1 reaching, 2 on the paint, 3 holding the set, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Sail, Win, Lose };

    void begin();
    void update(float dt);
    void draw();
    void pilot(float& sheet, float& helm);
    void helm(float& sheet, float& helmIn);
    bool onMark() const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void place(const gs::Mipped& m, float wx, float wy, float h, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    float t_ = 0;
    float raceT_ = 0;
    float clock_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0, speed_ = 0;
    float camX_ = 0, camY_ = 0;
    float hold_ = 0;
    float still_ = 0;
    float tone_ = 0;
    std::string why_;
};

}  // namespace headermark
