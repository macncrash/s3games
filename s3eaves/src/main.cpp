// S3 EAVES
//   s3eaves                 play
//   s3eaves --sim           autopilot climbs to the far ladder
//   s3eaves --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/eaves.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        eaves::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 70; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    eaves::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false;
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.roof() >= 4) {
            save(sys, "roofs.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        std::printf("fell short  roof %d  falls %d  x %.0f  y %.0f  (%.1f s)\n", cart.roof(), cart.falls(), cart.heroX(),
                    cart.heroY(), frames / 60.0);
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
            std::printf("S3 EAVES %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3eaves [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<eaves::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
