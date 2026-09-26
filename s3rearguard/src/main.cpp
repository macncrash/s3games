// S3 REARGUARD
//   s3rearguard                 play
//   s3rearguard --sim           autopilot walks the column home
//   s3rearguard --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/rearguard.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        rearguard::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 36; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    rearguard::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool fight = false, rise = false, home = false;
    int frames = 0;
    const int limit = 60 * 100;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !fight && frames > 50) {
            save(sys, "fight.png");
            fight = true;
        } else if (m == 2 && !rise) {
            save(sys, "rise.png");
            rise = true;
        } else if (m == 3 && !home && cart.victoryAge() > 1.15f) {
            save(sys, "home.png");
            home = true;
        }
    }
    if (!home) save(sys, "end.png");
    std::printf("S3 REARGUARD  %s  score %d  column %d  (%.1f s)\n", cart.won() ? "THE COLUMN IS HOME" : "THE COLUMN BREAKS",
                cart.score(), cart.column(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 REARGUARD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3rearguard [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<rearguard::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
