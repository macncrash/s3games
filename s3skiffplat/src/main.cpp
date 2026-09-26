// S3 SKIFF PLAT
//   s3skiffplat                 stop level with the platform
//   s3skiffplat --sim           autopilot must hold the skiff level with it
//   s3skiffplat --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/skiff.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        skiffplat::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    skiffplat::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool channel = false, along = false, held = false;
    int frames = 0;
    const int limit = 60 * 80;
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
            "S3 SKIFF PLAT  FAIL  %s  x %.2f  y %.1f  hdg %.0f  spd %.2f  (%.1f s)\n",
            why, cart.x(), cart.y(), cart.heading() * 57.2958f, cart.speed(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 SKIFF PLAT  LEVEL  stopped level with the platform  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 SKIFF PLAT %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3skiffplat [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<skiffplat::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
