// S3 TUG
//   s3tug                 berth the ship
//   s3tug --sim           autopilot must make the slip without a piling
//   s3tug --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/tug.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        tug::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    tug::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && frames == 120) {
            save(sys, "harbor.png");
            mid = true;
        }
    }
    if (shotDir) save(sys, "end.png");
    if (!cart.won() || cart.piled()) {
        std::printf("S3 TUG  FAIL  %s  x %.1f  y %.1f  hdg %.2f  spd %.2f  (%.1f s)\n", cart.piled() ? "piling" : "adrift",
                    cart.x(), cart.y(), cart.heading(), cart.speed(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 TUG  BERTHED  ship in the slip  pilings clear  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 TUG %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3tug [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<tug::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
