// S3 PLOW LOCK
//   s3plowlock                 play
//   s3plowlock --sim           autopilot must clear the lock
//   s3plowlock --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/plow.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        plowlock::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    plowlock::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool approach = false, chamber = false, clear = false;
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!approach && m == 1 && frames > 20) {
            save(sys, "approach.png");
            approach = true;
        } else if (!chamber && m == 2) {
            save(sys, "chamber.png");
            chamber = true;
        } else if (!clear && m == 3) {
            save(sys, "clear.png");
            clear = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "missed the end";
        std::printf("S3 PLOW LOCK  FAIL  %s  (%.1f s)\n", why, frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 PLOW LOCK  PASSED  the lock without scraping a gate  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 PLOW LOCK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3plowlock [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<plowlock::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
