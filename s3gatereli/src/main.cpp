// S3 GATE RELIEF
//   s3gatereli                 play
//   s3gatereli --sim           the sentry holds until the relief bell
//   s3gatereli --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/gate.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        gate::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    gate::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool sawWatch = false, sawBell = false, sawEnd = false;
    int frames = 0;
    const int limit = 60 * 100;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !sawWatch && frames > 90) {
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
    if (!sawEnd) save(sys, "end.png");
    const char* word = cart.won() ? "THE WATCH HELD UNTIL THE RELIEF BELL" : cart.reason();
    std::printf("S3 GATE RELIEF  %s  gate %d  score %d  (%.1f s)\n", word, cart.gate(), cart.score(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 GATE RELIEF %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3gatereli [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<gate::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
