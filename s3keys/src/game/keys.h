// S3 KEYS — one song. Miss five and the tune dies.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace keys {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KEYS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int hits() const { return hits_; }
    int misses() const { return misses_; }

private:
    enum class Mode { Title, Play, Dead, Clear };

    struct Note {
        double time = 0;
        int step = 0;
        int lane = 0;
        int midi = 60;
        bool done = false;
    };
    struct Pop {
        float x = 0, y = 0;
        double born = 0;
        int kind = 0;
    };

    void startSong();
    void updatePlay();
    void miss(int lane);
    void win();
    void die();
    void music(double t);
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog = 0);
    void noteOn(int ch, int midi, float vol);
    void click(bool accent);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool chartOk_ = true;
    int hits_ = 0;
    int perfect_ = 0;
    int misses_ = 0;
    int combo_ = 0;
    int bestCombo_ = 0;
    int songFrame_ = 0;
    double t_ = 0;
    int clickN_ = 0;
    int eighthN_ = 0;
    int fanStep_ = -1;
    double clock_ = 0;
    double clearAt_ = 0;
    double deadAt_ = 0;
    double clickLeft_ = 0;
    double melOff_ = 0;
    double bassOff_ = 0;
    bool melOn_ = false;
    bool bassOn_ = false;
    float keyLit_[kLanes] = {};
    std::vector<Note> notes_;
    std::vector<Pop> pops_;
};

}  // namespace keys
