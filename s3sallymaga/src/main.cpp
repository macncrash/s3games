// S3 SALLY MAGA
//   s3sallymaga                 hold the sally
//   s3sallymaga --sim           autopilot keeps a round past the raid
//   s3sallymaga --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/sally.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        sallymaga::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    sallymaga::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false, end = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.marker() == 1 && cart.down() >= 1 && cart.raidTime() > 2.f) {
            save(sys, "raid.png");
            mid = true;
        }
        if (cart.over() && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (cart.won() && cart.rounds() > 0 && cart.down() == 5) {
        std::printf("S3 SALLY MAGA  THE MAGAZINE OUTLASTS THE RAID  rounds %d  down %d\n", cart.rounds(),
                    cart.down());
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 SALLY MAGA  THE RAID IS NOT DONE  %s  rounds %d  down %d\n", cart.result(), cart.rounds(),
                cart.down());
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
            std::printf("S3 SALLY MAGA %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3sallymaga [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<sallymaga::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
