// S3 KART
//   s3kart                 eight laps, from the back of the pack
//   s3kart --sim           autopilot must finish ahead
//   s3kart --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/kart.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        kart::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 90; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    kart::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false;
    int frames = 0;
    const int limit = 60 * 200;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.laps() >= 1) {
            save(sys, "race.png");
            mid = true;
        }
    }
    save(sys, "finish.png");
    if (cart.won()) {
        std::printf("S3 KART  WIN  finished 8 laps ahead of the pack\n");
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 KART  FAIL  P%d  laps %d/8  (%.1f s)\n", cart.place(), cart.laps(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 KART %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3kart [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<kart::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
