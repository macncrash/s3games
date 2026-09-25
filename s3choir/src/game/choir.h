// S3 CHOIR — three entries. They land together or the piece stops.
#pragma once
#include "console/system.h"
#include "pictures.h"

namespace choir {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CHOIR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int phrase() const { return phrase_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 phrase, 2 the landing, 3 the piece stopped, 4 anthem sung
    int marker() const;

private:
    enum class Mode { Title, Phrase, Pause, Resolve, Stop, Victory };

    void enterTitle();
    void startAnthem();
    void beginPhrase(int p);
    void phraseFrame();
    void succeed();
    void fail(const char* why);
    void cueVoice(int i);
    void breath(int i);
    bool possible() const;
    bool landed() const;
    int spread() const;
    float progress(int i) const;
    bool hot(int i) const;
    void silence();
    void swell();
    void choke();
    void audioTail();

    void draw();
    void backdrop();
    void board();
    void people();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void hudR(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, int pal, bool feet, int fog = 0);
    void ruleAt(const gs::Image& img, int x, int y, int w, int h, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int phrase_ = 0;
    int tick_ = 0;
    int hold_ = 0;
    int anim_ = 0;
    int score_ = 0;
    int lastSpread_ = 0;
    int breath_ = 0;
    bool cued_[3] = {};
    int cueAt_[3] = {};
    int arr_[3] = {};
    char reason_[40] = {};
};

}  // namespace choir
