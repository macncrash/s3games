// S3 MOSAIC — slide the tiles until the picture is whole.
#pragma once

#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace mosaic {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MOSAIC"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int moves() const { return moves_; }
    int picture() const { return picture_; }
    // 0 title, 1 scrambling or sliding, 2 picture just completed, 3 victory
    int marker() const;

private:
    enum class Phase { Title, Study, Pop, Scramble, Play, Seal, Banner, Victory };

    struct Anim {
        bool on = false;
        int id = 0;
        int from = 0;
        int to = 0;
        int t = 0;
        int n = 1;
    };

    void update();
    void draw();
    void drawTitle();
    void drawBoard();
    void drawTiles();
    void drawChevrons();
    void drawHud();
    void backdrop();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void blit(const gs::Image& img, int x, int y, int pal, int w = -1, int h = -1);
    void beginPicture(int index);
    void buildPath();
    bool slide(int dir, bool count);
    void undo();
    void snapScramble();
    void senseDir();
    void pumpAudio();
    void click(int id);
    void bonk();
    void fan(int step);
    void lamp();
    bool confirm() const;
    bool arranged() const;
    int cellX(int i) const { return OX + (i % COLS) * PITCH; }
    int cellY(int i) const { return OY + (i / COLS) * PITCH; }
    int studyLimit() const { return bot_ ? 18 : 150; }
    int popLimit() const { return bot_ ? 6 : 12; }
    int sealLimit() const { return bot_ ? 8 : 16; }
    int bannerLimit() const { return bot_ ? 36 : 80; }
    int slideFrames() const { return bot_ ? 3 : 6; }
    uint32_t rnd();

    gs::System* sys_ = nullptr;
    Art art_{};
    Phase phase_ = Phase::Title;
    Anim anim_{};
    int grid_[CELLS] = {};
    int blank_ = BLANK;
    int picture_ = 0;
    int moves_ = 0;
    int picMoves_ = 0;
    int step_ = 0;
    int t_ = 0;
    int held_ = -1;
    int heldT_ = 0;
    int want_ = -1;
    int clickLeft_ = 0;
    int bonkLeft_ = 0;
    int fanLeft_ = 0;
    uint32_t rng_ = 1;
    bool bot_ = false;
    bool won_ = false;
    bool over_ = false;
    bool paused_ = false;
    bool pauseLatch_ = false;
    bool undoLatch_ = false;
    std::vector<int> path_;
    std::vector<int> solve_;
    std::vector<int> hist_;
};

}  // namespace mosaic
