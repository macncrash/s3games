// S3 KARTBUOY — round the harbor buoys and beat the other crew back to the dock.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace buoy {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KARTBUOY"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float raceTime() const { return time_; }
    float crewTime() const { return crew_; }
    int marks() const { return marks_; }
    // 0 title, 1 on the quay, 2 docked (win or lose)
    int marker() const;

private:
    enum class Mode { Title, Race, Pause, Win, Lose };

    void startRace();
    void updateRace();
    void botDrive(float& steer, bool& accel, bool& brake);
    void readPad(float& steer, bool& accel, bool& brake);
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void spr(const gs::Mipped& m, float sx, float sy, float h, int pal, bool flip = false, bool shadow = false);
    void blip(float freq);
    void fanfare();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool leftDock_ = false;
    bool engine_ = false;
    int marks_ = 0;
    int wp_ = 0;
    int stuck_ = 0;
    float x_ = 0, y_ = 0, hdg_ = 0, speed_ = 0;
    float time_ = 0;
    float crew_ = 17.50f;
    float t_ = 0;
    float beep_ = 0;
    float fanT_ = 0;
    int fanStep_ = -1;
    float camX_ = 0, camY_ = 0;
};

}  // namespace buoy
