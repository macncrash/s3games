#include "game/solitaire.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <queue>
#include <unordered_map>

namespace sol {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr int COLS = 7;
constexpr int COL_PITCH = 44;
constexpr int COL_X = (gs::SCREEN_W - COLS * COL_PITCH) / 2;
constexpr int TAB_Y = 72;
constexpr int FOUND_Y = 12;
constexpr int FOUND_PITCH = 44;
constexpr int FOUND_X = (gs::SCREEN_W - (NSUIT * FOUND_PITCH - (FOUND_PITCH - CARD_W))) / 2;
constexpr int CARD_BOTTOM = 212;

int suitOf(uint8_t c) { return c & 3; }
int rankOf(uint8_t c) { return c >> 2; }
bool red(uint8_t c) {
    int s = suitOf(c);
    return s == 1 || s == 2;
}

const char* rankName(int rank) {
    static const char* n = "A234567";
    return n + rank;
}

std::string cardName(uint8_t c) {
    static const char* s = "SHDC";
    std::string o;
    o.push_back(rankName(rankOf(c))[0]);
    o.push_back(s[suitOf(c)]);
    return o;
}

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.12f;
    p.op[0] = {1.f, 1.f, 0.004f, 0.18f, 0.f, 0.2f};
    p.op[1] = {2.f, 0.35f, 0.01f, 0.2f, 0.f, 0.18f};
    p.op[2] = {3.f, 0.18f, 0.01f, 0.16f, 0.f, 0.16f};
    p.op[3] = {4.2f, 0.1f, 0.01f, 0.14f, 0.f, 0.14f};
    p.vol = 0.2f;
    return p;
}

// The one deal. Buried card first, face card last. 0xff ends the column.
// Suits are S H D C. Ranks are A..7 packed as (rank << 2) | suit.
constexpr uint8_t kDeal[7][8] = {
    {0x18, 0x15, 0x10, 0xff, 0xff, 0xff, 0xff, 0xff},
    {0x19, 0x14, 0x11, 0x0c, 0x09, 0x04, 0xff, 0xff},
    {0x1a, 0x17, 0x12, 0x0f, 0x0a, 0x07, 0xff, 0xff},
    {0x1b, 0x16, 0x13, 0x0e, 0x0b, 0x06, 0xff, 0xff},
    {0x00, 0x0d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {0x02, 0x03, 0x08, 0x05, 0xff, 0xff, 0xff, 0xff},
};

}  // namespace

void Game::layout(Table& t) const {
    for (int i = 0; i < COLS; i++) {
        t.col[i].clear();
        for (uint8_t c : kDeal[i]) {
            if (c == 0xff) break;
            t.col[i].push_back(c);
        }
    }
    for (int s = 0; s < NSUIT; s++) t.found[s] = -1;
}

bool Game::cleared(const Table& t) const {
    for (const auto& c : t.col)
        if (!c.empty()) return false;
    return true;
}

int Game::homeOf(const Table& t) const {
    int n = 0;
    for (int f : t.found)
        if (f >= 0) n += f + 1;
    return n;
}

bool Game::legal(const Table& t, Move m) const {
    if (m.src < 0 || m.src >= COLS) return false;
    const auto& src = t.col[m.src];
    if (m.at < 0 || m.at >= int(src.size())) return false;
    if (m.kind == 0) {
        if (m.at != int(src.size()) - 1) return false;
        uint8_t c = src.back();
        int r = rankOf(c), s = suitOf(c);
        if (r == 0) return t.found[s] < 0;
        return t.found[s] == r - 1;
    }
    if (m.dst < 0 || m.dst >= COLS || m.dst == m.src) return false;
    for (int k = m.at; k + 1 < int(src.size()); k++) {
        uint8_t a = src[k], b = src[k + 1];
        if (rankOf(b) + 1 != rankOf(a) || red(b) == red(a)) return false;
    }
    uint8_t moving = src[m.at];
    const auto& dst = t.col[m.dst];
    if (dst.empty()) return rankOf(moving) == NRANK - 1;
    uint8_t top = dst.back();
    return rankOf(moving) + 1 == rankOf(top) && red(moving) != red(top);
}

void Game::apply(Table& t, Move m) const {
    if (m.kind == 0) {
        uint8_t c = t.col[m.src].back();
        t.col[m.src].pop_back();
        t.found[suitOf(c)] = int8_t(rankOf(c));
        return;
    }
    auto& src = t.col[m.src];
    auto& dst = t.col[m.dst];
    dst.insert(dst.end(), src.begin() + m.at, src.end());
    src.erase(src.begin() + m.at, src.end());
}

bool Game::anyMove(const Table& t) const {
    for (int i = 0; i < COLS; i++) {
        if (t.col[i].empty()) continue;
        Move up{0, i, int(t.col[i].size()) - 1, -1};
        if (legal(t, up)) return true;
        for (int at = 0; at < int(t.col[i].size()); at++) {
            for (int j = 0; j < COLS; j++) {
                if (j == i) continue;
                if (legal(t, Move{1, i, at, j})) return true;
            }
        }
    }
    return false;
}

std::vector<Game::Move> Game::solve(const Table& start) const {
    struct Node {
        Table t;
        int parent = -1;
        Move mv;
    };
    struct Item {
        int score;
        int seq;
        int id;
        bool operator<(const Item& o) const {
            if (score != o.score) return score < o.score;
            return seq > o.seq;
        }
    };

    std::vector<Node> nodes;
    nodes.push_back(Node{start, -1, {}});
    std::unordered_map<std::string, int> seen;
    auto keyOf = [](const Table& t) {
        std::string k;
        k.reserve(48);
        for (int i = 0; i < COLS; i++) {
            if (i) k.push_back('|');
            for (uint8_t c : t.col[i]) k.push_back(char(c + 1));
        }
        k.push_back('#');
        for (int f : t.found) k.push_back(char(f + 2));
        return k;
    };
    seen.emplace(keyOf(start), 0);
    std::priority_queue<Item> pq;
    pq.push({0, 0, 0});
    int seq = 1;
    int end = -1;
    while (!pq.empty() && int(nodes.size()) < 80000) {
        int id = pq.top().id;
        pq.pop();
        if (cleared(nodes[id].t)) {
            end = id;
            break;
        }
        // Copy first. push_back below can reallocate nodes and would dangle a reference.
        Table t = nodes[id].t;
        std::vector<Move> ms;
        for (int i = 0; i < COLS; i++) {
            if (t.col[i].empty()) continue;
            Move up{0, i, int(t.col[i].size()) - 1, -1};
            if (legal(t, up)) ms.push_back(up);
        }
        for (int i = 0; i < COLS; i++) {
            if (t.col[i].empty()) continue;
            for (int at = 0; at < int(t.col[i].size()); at++) {
                for (int j = 0; j < COLS; j++) {
                    if (j == i) continue;
                    Move mv{1, i, at, j};
                    if (legal(t, mv)) ms.push_back(mv);
                }
            }
        }
        int depth = 0;
        for (int p = id; p > 0; p = nodes[p].parent) depth++;
        for (Move mv : ms) {
            Table nxt = t;
            apply(nxt, mv);
            std::string k = keyOf(nxt);
            if (seen.count(k)) continue;
            int nid = int(nodes.size());
            seen.emplace(k, nid);
            nodes.push_back(Node{std::move(nxt), id, mv});
            int fc = homeOf(nodes.back().t);
            pq.push({fc * 100 - (depth + 1), seq++, nid});
        }
    }
    std::vector<Move> path;
    if (end < 0) return path;
    for (int i = end; i > 0; i = nodes[i].parent) path.push_back(nodes[i].mv);
    std::reverse(path.begin(), path.end());
    Table check = start;
    for (Move mv : path) {
        if (!legal(check, mv)) return {};
        apply(check, mv);
    }
    if (!cleared(check)) return {};
    return path;
}

void Game::startDeal() {
    layout(table_);
    moves_ = 0;
    home_ = 0;
    cursor_ = 4;
    hold_ = -1;
    holdAt_ = 0;
    fly_.clear();
    flyT_ = 1;
    undo_.clear();
    hint_ = 0;
    hintOn_ = false;
    won_ = false;
    over_ = false;
    celebrate_ = 0;
    gap_ = 0;
    fanStep_ = -1;
    mode_ = Mode::Play;
    script_.clear();
    scriptAt_ = 0;
    if (bot_) {
        script_ = solve(table_);
        intro_ = 28;
    } else {
        intro_ = 0;
    }
}

bool Game::commit(Move m) {
    if (flyT_ < 1.f) return false;
    if (!legal(table_, m)) return false;
    std::vector<uint8_t> run;
    if (m.kind == 0) run.push_back(table_.col[m.src].back());
    else run.assign(table_.col[m.src].begin() + m.at, table_.col[m.src].end());

    std::vector<std::pair<float, float>> from;
    int n = int(table_.col[m.src].size());
    for (int i = 0; i < int(run.size()); i++) {
        float x, y;
        int index = (m.kind == 0) ? n - 1 : m.at + i;
        cell(m.src, index, n, x, y);
        from.push_back({x, y});
    }
    if (!bot_) {
        if (undo_.size() >= 48) undo_.erase(undo_.begin());
        undo_.push_back(table_);
    }
    apply(table_, m);
    moves_++;
    home_ = homeOf(table_);
    fly_.clear();
    int dn = (m.kind == 1) ? int(table_.col[m.dst].size()) : 0;
    int base = (m.kind == 1) ? dn - int(run.size()) : 0;
    for (int i = 0; i < int(run.size()); i++) {
        Flight f;
        f.id = run[i];
        f.x0 = from[i].first;
        f.y0 = from[i].second;
        if (m.kind == 0) {
            foundAt(suitOf(run[i]), f.x1, f.y1);
        } else {
            cell(m.dst, base + i, dn, f.x1, f.y1);
        }
        fly_.push_back(f);
    }
    flyT_ = 0;
    hold_ = -1;
    blip(m.kind == 0 ? 2 : 0);
    return true;
}

void Game::onLand() {
    fly_.clear();
    flyT_ = 1;
    home_ = homeOf(table_);
    if (cleared(table_)) {
        mode_ = Mode::Victory;
        won_ = true;
        celebrate_ = 54;
        fanfare();
        return;
    }
    if (bot_) {
        gap_ = 3;
        if (scriptAt_ >= script_.size()) {
            over_ = true;
            won_ = false;
        }
        return;
    }
    if (!anyMove(table_)) mode_ = Mode::Stuck;
}

void Game::blip(int kind) {
    float f = kind == 2 ? 880.f : kind == 1 ? 640.f : 392.f;
    sys_->apu.tone(0, f, 0.055f);
    beep_ = 0.045f;
}

void Game::fanfare() {
    sys_->apu.setPatch(0, bellPatch());
    fanStep_ = 0;
    fanT_ = 0;
}

void Game::fanTick() {
    if (fanStep_ < 0) return;
    fanT_ += DT;
    if (fanT_ < 0.11f) return;
    fanT_ = 0;
    static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
    if (fanStep_ < 4) sys_->apu.keyOn(0, notes[fanStep_], 0.2f);
    else sys_->apu.keyOff(0);
    fanStep_++;
    if (fanStep_ > 8) fanStep_ = -1;
}

std::string Game::hintLine(Move m) const {
    if (m.src < 0 || m.src >= COLS || table_.col[m.src].empty()) return "NO HINT";
    if (m.at < 0 || m.at >= int(table_.col[m.src].size())) return "NO HINT";
    if (m.kind == 0) return "SEND " + cardName(table_.col[m.src].back()) + " UP";
    return "MOVE " + cardName(table_.col[m.src][m.at]) + " TO " + std::to_string(m.dst + 1);
}

void Game::playHuman(const gs::Pad& pad) {
    if (pad.pressed(gs::BTN_MODE)) {
        if (hold_ >= 0) {
            hold_ = -1;
            blip(1);
        } else {
            mode_ = Mode::Pause;
            blip(0);
        }
        return;
    }
    if (pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        blip(0);
        return;
    }
    if (pad.pressed(gs::BTN_X)) {
        if (hold_ >= 0 || undo_.empty()) {
            blip(0);
            if (hold_ >= 0) hold_ = -1;
            return;
        }
        table_ = undo_.back();
        undo_.pop_back();
        if (moves_ > 0) moves_--;
        home_ = homeOf(table_);
        hintOn_ = false;
        hint_ = 0;
        blip(1);
        return;
    }
    if (pad.pressed(gs::BTN_Z)) {
        auto path = solve(table_);
        if (path.empty()) {
            hintOn_ = false;
            hint_ = 90;
            hintMv_ = {};
            hintMv_.src = -1;
        } else {
            hintMv_ = path[0];
            hintOn_ = true;
            hint_ = 120;
            cursor_ = path[0].src;
        }
        blip(1);
        return;
    }
    if (pad.pressed(gs::BTN_LEFT) || pad.pressed(gs::BTN_RIGHT)) {
        int d = pad.pressed(gs::BTN_RIGHT) ? 1 : -1;
        cursor_ = (cursor_ + d + COLS) % COLS;
        blip(1);
    }
    if (hold_ >= 0) {
        const auto& src = table_.col[hold_];
        if (pad.pressed(gs::BTN_UP) && holdAt_ > 0) {
            bool run = true;
            for (int k = holdAt_ - 1; k + 1 < int(src.size()); k++) {
                uint8_t a = src[k], b = src[k + 1];
                if (rankOf(b) + 1 != rankOf(a) || red(b) == red(a)) run = false;
            }
            if (run) {
                holdAt_--;
                blip(1);
            }
        }
        if (pad.pressed(gs::BTN_DOWN) && holdAt_ + 1 < int(src.size())) {
            holdAt_++;
            blip(1);
        }
    }
    auto sendUp = [&]() {
        int col = hold_ >= 0 ? hold_ : cursor_;
        if (hold_ >= 0 && holdAt_ != int(table_.col[hold_].size()) - 1) {
            blip(0);
            return;
        }
        if (table_.col[col].empty()) {
            blip(0);
            return;
        }
        Move m{0, col, int(table_.col[col].size()) - 1, -1};
        if (!commit(m)) blip(0);
    };
    if (hold_ < 0 && pad.pressed(gs::BTN_UP)) sendUp();
    if (pad.pressed(gs::BTN_B)) sendUp();
    if (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A)) {
        if (hold_ < 0) {
            if (table_.col[cursor_].empty()) {
                blip(0);
                return;
            }
            hold_ = cursor_;
            holdAt_ = int(table_.col[cursor_].size()) - 1;
            blip(1);
            return;
        }
        if (cursor_ == hold_) {
            hold_ = -1;
            blip(1);
            return;
        }
        Move m{1, hold_, holdAt_, cursor_};
        if (!commit(m)) blip(0);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.12f, 0.22f, 0.14f);
    sys.apu.setPatch(0, bellPatch());
    if (bot_) startDeal();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) sys.apu.tone(0, 0, 0);
    }
    fanTick();
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            blip(2);
            startDeal();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Play;
            blip(1);
        } else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            blip(0);
        }
    } else if (mode_ == Mode::Stuck) {
        if (pad.pressed(gs::BTN_START)) startDeal();
        else if (pad.pressed(gs::BTN_MODE)) mode_ = Mode::Title;
    } else if (mode_ == Mode::Victory) {
        if (celebrate_ > 0) celebrate_--;
        else if (bot_) over_ = true;
        if (!bot_ && pad.pressed(gs::BTN_START)) startDeal();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) mode_ = Mode::Title;
    } else if (mode_ == Mode::Play) {
        if (flyT_ < 1.f) {
            flyT_ += 0.14f;
            if (flyT_ >= 1.f) onLand();
        } else if (bot_) {
            if (intro_ > 0) intro_--;
            else if (gap_ > 0) gap_--;
            else if (scriptAt_ < script_.size()) {
                Move m = script_[scriptAt_++];
                if (!commit(m)) {
                    over_ = true;
                    won_ = false;
                }
            } else if (!won_) {
                over_ = true;
            }
        } else {
            playHuman(pad);
        }
    }
    if (hint_ > 0) hint_--;
    draw();
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0;
    for (unsigned char c : s) {
        if (c <= 32 || c >= 128) width += 10.f * scale;
        else width += art_.glyph[c - 32].w * scale + scale;
    }
    x -= width * 0.5f;
    for (unsigned char c : s) {
        if (c <= 32 || c >= 128) {
            x += 10.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float h = g.h * scale;
        float w = g.w * scale;
        spr(g, x, y - h * 0.5f, w, h, pal, false);
        x += w + scale;
    }
}

void Game::spr(const gs::Mipped& m, float x, float y, float w, float h, int pal, bool shadow) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::cardAt(uint8_t id, float x, float y) {
    if (id >= NCARD) return;
    spr(art_.face[id], x, y, CARD_W, CARD_H, PAL_FACE, false);
    spr(art_.shadow, x + 3, y + 4, CARD_W, CARD_H, 0, true);
}

int Game::cascade(int n) const {
    if (n <= 1) return 14;
    int avail = CARD_BOTTOM - TAB_Y - CARD_H;
    int s = 14;
    if ((n - 1) * s > avail) s = std::max(8, avail / (n - 1));
    return s;
}

void Game::cell(int col, int index, int n, float& x, float& y) const {
    x = float(COL_X + col * COL_PITCH);
    y = float(TAB_Y + index * cascade(n));
}

void Game::foundAt(int suit, float& x, float& y) const {
    x = float(FOUND_X + suit * FOUND_PITCH);
    y = float(FOUND_Y);
}

bool Game::inFlight(uint8_t id) const {
    if (flyT_ >= 1.f) return false;
    for (const Flight& f : fly_)
        if (f.id == id) return true;
    return false;
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.setFogColor(gs::rgb4(0, 3, 1));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineBackdrop[y] = gs::rgb4(0, 4, 2);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    auto drawCard = [&](uint8_t id, float x, float y) { cardAt(id, x, y); };

    // Banners sit in front of the cloth.
    if (mode_ == Mode::Title) {
        text("S3 SOLITAIRE", 160, 26, 1.f, PAL_GOLD);
        float aw = 58, ah = 80;
        float pitch = 66;
        float x0 = 160 - (4 * pitch - (pitch - aw)) * 0.5f;
        for (int s = 0; s < 4; s++) {
            float x = x0 + s * pitch;
            float y = 46;
            spr(art_.shadow, x + 3, y + 5, aw, ah, 0, true);
            spr(art_.face[s], x, y, aw, ah, PAL_FACE, false);
        }
        hudC(16, "ACES THROUGH SEVENS", PAL_HUD);
        hudC(18, "BUILD DOWN. RED ON BLACK.", PAL_HUD);
        hudC(20, "X OR UP SENDS A CARD HOME.", PAL_HUD);
        hudC(22, "A SEVEN OPENS AN EMPTY FILE.", PAL_HUD);
        hudC(24, "ONE DEAL. CLEAR THE TABLEAU.", PAL_GOLD);
        if ((sys_->frame / 30) % 2 == 0) hudC(26, "PRESS START", PAL_WIN);
        return;
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 104, 1.15f, PAL_GOLD);
        hudC(16, "START RESUMES", PAL_HUD);
        hudC(17, "ESC TO THE TITLE", PAL_HUD);
    } else if (mode_ == Mode::Stuck) {
        text("NO MOVE", 160, 96, 1.1f, PAL_ALERT);
        hudC(16, "START DEALS IT AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        text("TABLEAU CLEAR", 160, 92, 0.92f, PAL_WIN);
        char buf[40];
        std::snprintf(buf, sizeof buf, "MOVES %d", moves_);
        hudC(15, buf, PAL_GOLD);
        if (!bot_) hudC(17, "START DEALS IT AGAIN", PAL_HUD);
    } else if (intro_ > 0 && bot_) {
        text("ONE DEAL", 160, 108, 1.f, PAL_GOLD);
    }

    // Foundations, then the tableau. Earlier sprites are drawn on top.
    bool showHand = hold_ >= 0 && flyT_ >= 1.f && mode_ == Mode::Play;
    int handN = 0;
    if (showHand) handN = int(table_.col[hold_].size()) - holdAt_;

    auto skip = [&](int col, int index) {
        if (showHand && col == hold_ && index >= holdAt_) return true;
        if (col < 0 || col >= COLS || index >= int(table_.col[col].size())) return false;
        return inFlight(table_.col[col][index]);
    };

    // Flying and held cards first so they cover the piles.
    if (flyT_ < 1.f) {
        float u = std::clamp(flyT_, 0.f, 1.f);
        u = u * u * (3.f - 2.f * u);
        for (const Flight& f : fly_) {
            float x = f.x0 + (f.x1 - f.x0) * u;
            float y = f.y0 + (f.y1 - f.y0) * u;
            drawCard(f.id, x, y);
        }
    } else if (showHand) {
        int dest = cursor_;
        int destN = int(table_.col[dest].size());
        if (dest == hold_) destN = holdAt_;
        int previewN = (dest == hold_) ? holdAt_ + handN : destN + handN;
        int land = (dest == hold_) ? holdAt_ : destN;
        int stepCol = dest;
        for (int i = 0; i < handN; i++) {
            float x, y;
            cell(stepCol, land + i, previewN, x, y);
            if (dest == hold_) y -= 8;
            drawCard(table_.col[hold_][holdAt_ + i], x, y);
        }
        float cx, cy;
        cell(stepCol, land, previewN, cx, cy);
        if (dest == hold_) cy -= 8;
        bool ok = dest == hold_ || legal(table_, Move{1, hold_, holdAt_, dest});
        spr(art_.cursor, cx - 4, cy - 4, CARD_W + 8, CARD_H + 8, ok ? PAL_CURSOR : PAL_ALERT, false);
    }

    for (int s = 0; s < NSUIT; s++) {
        float x, y;
        foundAt(s, x, y);
        if (table_.found[s] >= 0) {
            uint8_t id = uint8_t((table_.found[s] << 2) | s);
            if (!inFlight(id)) drawCard(id, x, y);
        }
    }

    for (int c = 0; c < COLS; c++) {
        int n = int(table_.col[c].size());
        int drawN = n;
        if (showHand && c == hold_) drawN = holdAt_;
        // A hovered drop opens the column to the spacing it will have.
        int spaceN = drawN;
        if (showHand && c == cursor_ && c != hold_) spaceN = drawN + handN;
        if (showHand && c == hold_ && cursor_ == hold_) spaceN = n;
        for (int i = drawN - 1; i >= 0; i--) {
            if (skip(c, i)) continue;
            float x, y;
            cell(c, i, spaceN == drawN ? n : spaceN, x, y);
            if (showHand && c == hold_) cell(c, i, holdAt_, x, y);
            drawCard(table_.col[c][i], x, y);
        }
        if (drawN == 0) {
            float x, y;
            cell(c, 0, 1, x, y);
            spr(art_.file, x - 3, y - 3, CARD_W + 6, CARD_H + 6, PAL_FILE, false);
        }
    }

    for (int s = 0; s < NSUIT; s++) {
        float x, y;
        foundAt(s, x, y);
        spr(art_.slot[s], x - 3, y - 3, CARD_W + 6, CARD_H + 6, PAL_SLOT, false);
    }

    spr(art_.rule, float(COL_X), float(FOUND_Y + CARD_H + 4), float(COLS * COL_PITCH - (COL_PITCH - CARD_W)), 3,
        PAL_BRASS, false);

    if (!showHand && mode_ == Mode::Play) {
        float x, y;
        if (table_.col[cursor_].empty()) cell(cursor_, 0, 1, x, y);
        else cell(cursor_, int(table_.col[cursor_].size()) - 1, int(table_.col[cursor_].size()), x, y);
        spr(art_.cursor, x - 4, y - 4, CARD_W + 8, CARD_H + 8, PAL_CURSOR, false);
    }

    char buf[48];
    std::snprintf(buf, sizeof buf, "HOME %02d/28", home_);
    hud(1, 0, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "MV %02d", moves_);
    hud(32, 0, buf, PAL_GOLD);
    if (hint_ > 0) {
        std::string h = hintOn_ ? hintLine(hintMv_) : "NO HINT";
        hudC(26, h, PAL_GOLD);
    }
    if (mode_ == Mode::Play) {
        if (hold_ >= 0) hudC(27, "UP DOWN RUN   C DROP   Q UNDO", PAL_HUD);
        else hudC(27, "C PICK   X UP   Q UNDO   E HINT", PAL_HUD);
    }
}

}  // namespace sol
