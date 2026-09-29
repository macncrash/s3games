// S3 REDOUBT MAGA
//   s3redoubtmaga                 make the magazine outlast the raid
//   s3redoubtmaga --sim           autopilot keeps a round in the chest
//   s3redoubtmaga --sim --shots D also writes PNGs into D
//
//   Left and right pick a face. A or C fires. Enter starts.
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/maga.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        rmaga::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 12; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    rmaga::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool fight = false, end = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (shotDir && !fight && m == 1 && frames == 100) {
            save(sys, "glacis.png");
            fight = true;
        } else if (shotDir && !end && m == 2) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (shotDir && !end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 REDOUBT MAGA  THE MAGAZINE OUTLASTS THE RAID  rounds %d\n", cart.rounds());
        return 0;
    }
    std::printf("S3 REDOUBT MAGA  THE MAGAZINE FAILS  %s  rounds %d\n", cart.result(), cart.rounds());
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 REDOUBT MAGA %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3redoubtmaga [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<rmaga::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
