// S3 METRO SLIP
//   s3metroslip                 berth the water metro
//   s3metroslip --sim           autopilot berths before the tide turns
//   s3metroslip --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/metro.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        metroslip::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    metroslip::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool water = false, mouth = false, held = false;
    int frames = 0;
    const int limit = 60 * 80;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!water && m == 1 && frames > 40) {
            save(sys, "metro.png");
            water = true;
        } else if (!mouth && m == 2) {
            save(sys, "slip.png");
            mouth = true;
        } else if (!held && m == 3) {
            save(sys, "berth.png");
            held = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 METRO SLIP  FAIL  %s  y %.1f  off %.1f  spd %.2f  slip %d  (%.1f s)\n",
                    cart.over() ? cart.why() : "timed out", cart.y(), cart.x(), cart.speed(), cart.inSlip() ? 1 : 0,
                    frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 METRO SLIP %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3metroslip [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<metroslip::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
