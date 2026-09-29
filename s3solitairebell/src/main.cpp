// S3 SOLITAIRE BELL
//   s3solitairebell                 play until the bell rings
//   s3solitairebell --sim           autopilot rings the bell and leaves
//   s3solitairebell --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/solitairebell.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    solitairebell::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (!cart.rules()) {
        std::printf("S3 SOLITAIREBELL  DEAD  rules failed  bell silent  (0.0 s)\n");
        std::fflush(stdout);
        return 1;
    }
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (frames == 8) save(sys, "table.png");
    }
    save(sys, "end.png");
    if (cart.won() && cart.rung() && cart.deadTries() < 3 && cart.tryNo() >= 1 && cart.tryNo() <= 3 &&
        cart.built() == solitairebell::Game::kCards) {
        std::printf("S3 SOLITAIREBELL  BELL  left on try %d  before the third try died  (%.1f s)\n", cart.tryNo(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::fprintf(stderr, "s3solitairebell %s %s dead %d try %d rung %d built %d\n", cart.phase(), cart.reason(),
                 cart.deadTries(), cart.tryNo(), cart.rung() ? 1 : 0, cart.built());
    std::printf("S3 SOLITAIREBELL  DEAD  third try died  bell silent  (%.1f s)\n", frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SOLITAIRE BELL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3solitairebell [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<solitairebell::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
