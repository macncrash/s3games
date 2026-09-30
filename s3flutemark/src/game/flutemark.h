// S3 FLUTEMARK — play the flute until a finished mark ends the phrase.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace flutemark {

class Game : public gs::Cart {
public:
    static constexpr int kMarks = 4;

    const char* title() const override { return "S3 FLUTEMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }
    int notes() const;
    int misses() const { return misses_; }

private:
    enum class Mode { Title, Play, Miss, Win, Pause };

    void beginPhrase(bool keepMisses);
    void stepPlay();
    void blow();
    void drop();
    void finishMark();
    bool botBlow() const;
    float headX() const;
    void audio();
    void tone(float freq, float vol, int frames);
    void draw();
    void backdrop();
    void stage();
    void image(const gs::Mipped& m, float left, float top, float h, int pal, bool flip = false, int fog = 0);
    void spr(const gs::Mipped& m, float cx, float bottom, float h, int pal, bool flip = false, int fog = 0);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finished_ = false;
    bool did_[kMarks] = {};
    int finger_ = 0;
    int misses_ = 0;
    int hold_ = 0;
    int tick_ = 0;
    int missT_ = 0;
    int tone_ = 0;
    int fanT_ = 0;
    int breath_ = 0;
    int shake_ = 0;
    float t_ = 0;
    float phase_ = 0;
    float sayT_ = 0;
    char say_[24] = {};
};

}  // namespace flutemark
