// S3 CHOIRBELL
//   s3choirbell                 play
//   s3choirbell --sim           the choir rings the bell
//   s3choirbell --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/choir.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        choir::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    choir::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool phrase = false, ring = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !phrase) {
            save(sys, "phrase.png");
            phrase = true;
        } else if (m == 2 && !ring) {
            save(sys, "bell.png");
            ring = true;
        }
    }
    if (!ring) save(sys, "bell.png");
    std::printf("S3 CHOIRBELL  %s  the bell %s  tries left %d  (%.1f s)\n", cart.won() ? "PASS" : "FAIL",
                cart.won() ? "rang" : "stayed silent", cart.tries(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 CHOIRBELL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3choirbell [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<choir::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
