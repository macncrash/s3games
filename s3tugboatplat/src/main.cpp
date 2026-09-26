// S3 TUGBOAT PLAT
//   s3tugboatplat                 stop level with the platform
//   s3tugboatplat --sim           autopilot must hold level ahead of the other crew
//   s3tugboatplat --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/plat.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        tugplat::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    tugplat::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool channel = false, along = false, held = false;
    int frames = 0;
    const int limit = 60 * 70;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!channel && m >= 1 && frames > 30) {
            save(sys, "channel.png");
            channel = true;
        } else if (!along && m == 2) {
            save(sys, "alongside.png");
            along = true;
        } else if (!held && m == 3) {
            save(sys, "level.png");
            held = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf(
            "S3 TUGBOAT PLAT  FAIL  %s  x %.2f y %.1f hdg %.0f spd %.2f crew %.1f (%.1fs)\n",
            why, cart.x(), cart.y(), cart.heading() * 57.2958f, cart.speed(), cart.crewLeft(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf(
        "S3 TUGBOAT PLAT  LEVEL  stopped level with the platform ahead of the other crew  (%.1fs, %.1fs left)\n",
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
            std::printf("S3 TUGBOAT PLAT %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3tugboatplat [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<tugplat::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
