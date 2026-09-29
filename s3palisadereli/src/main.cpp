// S3 PALISADE RELIEF
//   s3palisadereli                 play
//   s3palisadereli --sim           autopilot holds the wall until the relief bell
//   s3palisadereli --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/palisade.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        palisade::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    palisade::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool sawWall = false, sawBell = false, sawEnd = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !sawWall && frames > 90) {
            save(sys, "wall.png");
            sawWall = true;
        } else if (m == 2 && !sawBell) {
            save(sys, "bell.png");
            sawBell = true;
        } else if (m == 3 && !sawEnd) {
            save(sys, "end.png");
            sawEnd = true;
        }
    }
    if (!sawEnd) save(sys, "end.png");
    const char* word = cart.won() ? "THE WATCH HELD UNTIL THE RELIEF BELL" : cart.reason();
    std::printf("S3 PALISADE RELIEF  %s  score %d  (%.1f s)\n", word, cart.score(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 PALISADE RELIEF %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3palisadereli [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<palisade::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
