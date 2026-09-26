// S3 RIDGE MAGA
//   s3ridgemaga                 hold the ridge
//   s3ridgemaga --sim           autopilot keeps one round past the raid
//   s3ridgemaga --sim --shots D also writes PNGs into D
#include "console/system.h"
#include "game/ridge.h"
#include "version.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        rmaga::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    rmaga::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false, hold = false, end = false;
    int frames = 0;
    const int limit = 60 * 45;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.marker() == 1 && cart.raidTime() > 8.f) {
            save(sys, "raid.png");
            mid = true;
        } else if (!hold && cart.raidTime() > 20.f) {
            save(sys, "hold.png");
            hold = true;
        }
        if (cart.over() && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 RIDGE MAGA  THE MAGAZINE OUTLASTS THE RAID  rounds %d  stopped %d\n", cart.rounds(),
                    cart.stopped());
        return 0;
    }
    std::printf("S3 RIDGE MAGA  THE RIDGE IS LOST  %s  rounds %d  stopped %d\n", cart.result(), cart.rounds(),
                cart.stopped());
    std::fputs(cart.trace().c_str(), stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 RIDGE MAGA %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3ridgemaga [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<rmaga::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
