// S3 FERRY GRASS
//   s3ferrygrass                 land on the grass
//   s3ferrygrass --sim           autopilot lands and comes to a full stop
//   s3ferrygrass --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/ferry.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        ferrygrass::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    ferrygrass::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool way = false, grass = false, stopped = false;
    int frames = 0;
    const int limit = 60 * 55;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!way && m >= 1 && frames > 24) {
            save(sys, "ferry.png");
            way = true;
        } else if (!grass && m == 2) {
            save(sys, "grass.png");
            grass = true;
        } else if (!stopped && m == 3) {
            save(sys, "stop.png");
            stopped = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() && cart.why()[0] ? cart.why() : "timed out";
        std::printf(
            "S3 FERRY GRASS  FAIL  %s  x %.1f  y %.1f  spd %.2f  crew %.1f  grass %d  (%.1f s)\n",
            why, cart.x(), cart.y(), cart.speed(), cart.crewLeft(), cart.onGrass() ? 1 : 0, frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf(
        "S3 FERRY GRASS  PASS  landed on the grass and came to a full stop ahead of the other crew  (%.1f s)\n",
        cart.seconds());
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
            std::printf("S3 FERRY GRASS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3ferrygrass [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<ferrygrass::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
