// S3 RAIL PASS
//   s3railpass                 roll the pass
//   s3railpass --sim           autopilot must beat the storm clock
//   s3railpass --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/pass.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        railpass::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    railpass::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool slope = false, drifts = false, mouth = false;
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!slope && m == 1) {
            save(sys, "slope.png");
            slope = true;
        } else if (!drifts && m == 2) {
            save(sys, "drifts.png");
            drifts = true;
        } else if (!mouth && m == 3) {
            save(sys, "mouth.png");
            mouth = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 RAIL PASS  FAIL  %s  z %.0f  clock %.1f  (%.1f s)\n", why, cart.meters(), cart.clockLeft(),
                    frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 RAIL PASS  CLEAR  cleared the pass before the storm clock  (%.1f s, %.1f s left)\n",
                cart.seconds(), cart.clockLeft());
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 RAIL PASS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3railpass [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<railpass::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
