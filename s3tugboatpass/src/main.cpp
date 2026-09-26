// S3 TUGBOAT PASS
//   s3tugboatpass                 take the tug through the pass
//   s3tugboatpass --sim           autopilot must clear the pass ahead of the other crew
//   s3tugboatpass --sim --shots D also writes PNGs into D
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
        tugpass::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    tugpass::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool pass = false, cut = false, lee = false;
    int frames = 0;
    const int limit = 60 * 110;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!pass && m == 1 && frames > 36) {
            save(sys, "pass.png");
            pass = true;
        } else if (!cut && m == 2) {
            save(sys, "cut.png");
            cut = true;
        } else if (!lee && m == 3) {
            save(sys, "lee.png");
            lee = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf(
            "S3 TUGBOAT PASS  FAIL  %s  x %.1f y %.1f hdg %.0f spd %.2f crew %.1f gap %.2f (%.1fs)\n",
            why, cart.x(), cart.y(), cart.heading() * 57.2958f, cart.speed(), cart.crewLeft(), cart.minGap(),
            frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf(
        "S3 TUGBOAT PASS  CLEAR  cleared the pass before the storm clock  (%.1fs, the other crew had %.1fs left)\n",
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
            std::printf("S3 TUGBOAT PASS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3tugboatpass [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<tugpass::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
