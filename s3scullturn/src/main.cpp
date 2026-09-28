// S3 SCULL TURN
//   s3scullturn                 row the three turns
//   s3scullturn --sim           autopilot finishes the leg upright
//   s3scullturn --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/scull.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        scullturn::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    scullturn::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool reach = false, bend = false, pair = false;
    int frames = 0;
    const int limit = 60 * 120;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!reach && m >= 1 && frames > 40) {
            save(sys, "reach.png");
            reach = true;
        } else if (!bend && m == 2) {
            save(sys, "bend.png");
            bend = true;
        } else if (!pair && m == 3) {
            save(sys, "pair.png");
            pair = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 SCULL TURN  FAIL  %s  turns %d  x %.1f  z %.1f  lean %.2f  (%.1f s)\n", why, cart.turns(),
                    cart.x(), cart.z(), cart.lean(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 SCULL TURN  PASS  made the three turns without tipping  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 SCULL TURN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3scullturn [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<scullturn::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
