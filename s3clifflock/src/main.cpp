// S3 CLIFF LOCK
//   s3clifflock                 take the cliff
//   s3clifflock --sim           autopilot must clear both gates and the far mark
//   s3clifflock --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/cliff.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        clifflock::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    clifflock::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool approach = false, lower = false, chamber = false, climb = false;
    int frames = 0;
    const int limit = 60 * 70;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!approach && m == 1 && frames > 20) {
            save(sys, "approach.png");
            approach = true;
        } else if (!lower && m == 2) {
            save(sys, "lower.png");
            lower = true;
        } else if (!chamber && m == 3) {
            save(sys, "chamber.png");
            chamber = true;
        } else if (!climb && m == 4) {
            save(sys, "climb.png");
            climb = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 CLIFF LOCK  FAIL  %s  x %.1f  spd %.1f  (%.1f s)\n", why, cart.x(), cart.speed(),
                    frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 CLIFF LOCK  CLEAR  passed the lock without scraping a gate  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 CLIFF LOCK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3clifflock [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<clifflock::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
