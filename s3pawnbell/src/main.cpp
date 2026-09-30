// S3 PAWNBELL
//   s3pawnbell                 a short pawn, one bell
//   s3pawnbell --sim           autopilot rings the bell before the third try dies
//   s3pawnbell --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/pawn.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    pawnbell::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    save(sys, "boot.png");
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    save(sys, "end.png");
    if (cart.won() && cart.rung() && cart.dead() < 3) {
        std::printf("S3 PAWNBELL  PASS  bell rang before the third try died  dead %d  (%.1f s)\n", cart.dead(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    const char* why = cart.over() ? cart.why() : "timed out";
    std::printf("S3 PAWNBELL  FAIL  %s  dead %d  (%.1f s)\n", why, cart.dead(), frames / 60.0);
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
            std::printf("S3 PAWNBELL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3pawnbell [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<pawnbell::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
