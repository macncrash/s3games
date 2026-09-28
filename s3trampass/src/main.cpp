// S3 TRAM PASS
//   s3trampass                 take the tram through the pass
//   s3trampass --sim           autopilot must clear the pass ahead of the other crew
//   s3trampass --sim --shots D also writes PNGs into D
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
        trampass::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    trampass::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (shotDir && m == 2 && frames == 90) save(sys, "pass.png");
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 TRAM PASS  FAIL  %s  z %.1f spd %.1f crew %.1f (%.1fs)\n", why, cart.meters(), cart.speed(),
                    cart.crewLeft(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf(
        "S3 TRAM PASS  CLEAR  cleared the pass before the storm clock  (%.1fs, the other crew had %.1fs left)\n",
        cart.seconds(), cart.crewLeft());
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
            std::printf("S3 TRAM PASS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3trampass [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<trampass::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
