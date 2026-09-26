// S3 DEPOT RELIEF
//   s3depotreli                 play
//   s3depotreli --sim           autopilot holds until the relief bell
//   s3depotreli --sim --shots D also writes PNGs into D
#include "console/system.h"
#include "game/depot.h"
#include "version.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        depot::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 50; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    depot::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool sawWatch = false, sawBell = false, sawEnd = false;
    int frames = 0;
    const int limit = 60 * 70;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !sawWatch && frames > 220) {
            save(sys, "watch.png");
            sawWatch = true;
        } else if (m == 2 && !sawBell) {
            save(sys, "bell.png");
            sawBell = true;
        } else if (m == 3 && !sawEnd) {
            save(sys, "end.png");
            sawEnd = true;
        }
    }
    if (shotDir && !sawEnd) save(sys, "end.png");
    std::printf("S3 DEPOT RELIEF  %s  stopped %d  bays %d  score %d  (%.1f s)\n", cart.reason(), cart.stopped(),
                cart.bays(), cart.score(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 DEPOT RELIEF %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3depotreli [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<depot::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
