// S3 TUGBOAT MARK
//   s3tugboatmark                 set the bow down on the mark
//   s3tugboatmark --sim           autopilot sets down before the end
//   s3tugboatmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/mark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        tugmark::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    tugmark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool way = false, painted = false, setting = false;
    int frames = 0;
    const int limit = 60 * 75;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!way && m >= 1 && frames > 30) {
            save(sys, "tug.png");
            way = true;
        } else if (!painted && m == 2) {
            save(sys, "mark.png");
            painted = true;
        } else if (!setting && m == 3) {
            save(sys, "set.png");
            setting = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf(
            "S3 TUGBOAT MARK  FAIL  %s  x %.1f y %.1f hdg %.2f spd %.2f eng %.2f bow %.2f phase %d  (%.1f s)\n",
            why, cart.x(), cart.y(), cart.heading(), cart.speed(), cart.engine(), cart.bow(), cart.phase(),
            cart.seconds());
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 TUGBOAT MARK  SET DOWN  the bow is on the mark  the leg is made  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 TUGBOAT MARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3tugboatmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<tugmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
