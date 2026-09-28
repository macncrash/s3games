// S3 TRAM TURN
//   s3tramturn                 take the tram through three turns
//   s3tramturn --sim           autopilot stays upright before the other crew
//   s3tramturn --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/tram.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        tramturn::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    tramturn::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool t1 = false, t2 = false, t3 = false;
    int frames = 0;
    const int limit = 60 * 80;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!t1 && m >= 1 && frames > 40) {
            save(sys, "turn1.png");
            t1 = true;
        } else if (!t2 && m >= 2) {
            save(sys, "turn2.png");
            t2 = true;
        } else if (!t3 && m >= 3) {
            save(sys, "turn3.png");
            t3 = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf(
            "S3 TRAM TURN  FAIL  %s  turns %d  lean %.2f  spd %.1f  crew %.1f  x %.1f  y %.1f  (%.1f s)\n",
            why, cart.turns(), cart.lean(), cart.speed(), cart.crewLeft(), cart.x(), cart.y(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 TRAM TURN  CLEARED  three turns upright before the other crew  (%.1f s, %.1f s left on their clock)\n",
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
            std::printf("S3 TRAM TURN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3tramturn [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<tramturn::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
