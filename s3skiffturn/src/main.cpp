// S3 SKIFF TURN
//   s3skiffturn                 make the three turns without tipping
//   s3skiffturn --sim           autopilot takes the bends upright
//   s3skiffturn --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/turn.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        skiffturn::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    skiffturn::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool straight = false, bend = false, last = false;
    int frames = 0;
    const int limit = 60 * 110;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!straight && m >= 1 && frames > 36) {
            save(sys, "creek.png");
            straight = true;
        } else if (!bend && m == 2) {
            save(sys, "bend.png");
            bend = true;
        } else if (!last && m == 3) {
            save(sys, "steady.png");
            last = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf(
            "S3 SKIFF TURN  FAIL  %s  turns %d  heel %.0f  x %.1f  z %.1f  off %+.1f  hdg %.0f  spd %.1f  (%.1f s)\n",
            why, cart.turns(), cart.heel() * 57.2958f, cart.x(), cart.z(), cart.lateral(), cart.heading() * 57.2958f,
            cart.speed(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 SKIFF TURN  STEADY  made the three turns without tipping  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 SKIFF TURN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3skiffturn [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<skiffturn::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
