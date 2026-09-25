// S3 FORT
//   s3fort                 hold the gate
//   s3fort --sim           autopilot holds until the clock dies
//   s3fort --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/fort.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        fort::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 24; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    fort::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool wall = false, end = false;
    int frames = 0;
    const int limit = 60 * 48;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!wall && m == 1 && frames > 40) {
            save(sys, "gate.png");
            wall = true;
        } else if (!end && (m == 2 || m == 3)) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (shotDir && !end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 FORT  THE CLOCK DIES  gate %d  score %d\n", cart.gate(), cart.score());
        return 0;
    }
    std::printf("S3 FORT  THE GATE FALLS  gate %d  score %d\n", cart.gate(), cart.score());
    std::fprintf(stderr, "fail siege %.2f leaks %d kills %d over %d\n", cart.siege(), cart.leaks(), cart.kills(),
                 cart.over() ? 1 : 0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 FORT %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3fort [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<fort::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
