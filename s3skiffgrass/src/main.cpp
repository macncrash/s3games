// S3 SKIFF GRASS
//   s3skiffgrass                 land the skiff
//   s3skiffgrass --sim           autopilot lands on the grass and full-stops
//   s3skiffgrass --sim --shots D also writes PNGs into D
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
        skiffgrass::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    skiffgrass::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool water = false, grass = false, end = false;
    int frames = 0;
    const int limit = 60 * 100;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!water && m == 1 && frames > 30) {
            save(sys, "water.png");
            water = true;
        } else if (!grass && m == 2) {
            save(sys, "grass.png");
            grass = true;
        } else if (!end && m == 3) {
            save(sys, "end.png");
            end = true;
        }
    }
    save(sys, "stop.png");
    if (!cart.won()) {
        std::printf(
            "S3 SKIFF GRASS  FAIL  %s  x %.1f  y %.1f  hdg %.2f  spd %.2f  grass %d  end %d  (%.1f s)\n",
            cart.over() ? cart.why() : "timed out", cart.x(), cart.y(), cart.heading(), cart.speed(),
            cart.onGrass() ? 1 : 0, cart.inEnd() ? 1 : 0, frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SKIFF GRASS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3skiffgrass [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<skiffgrass::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
