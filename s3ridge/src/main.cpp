// S3 RIDGE
//   s3ridge                 hold the path
//   s3ridge --sim           autopilot holds the crest
//   s3ridge --sim --shots D also writes PNGs into D
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
    if (shotDir) {
        gs::System sys(true);
        ridge::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 24; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    ridge::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool fight = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (shotDir && !fight && cart.marker() == 1 && frames > 280) {
            save(sys, "fight.png");
            fight = true;
        }
    }
    if (shotDir) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 RIDGE  THE PATH HELD  stopped %d  stakes %d  score %d\n", cart.stopped(), cart.stakes(),
                    cart.score());
        return 0;
    }
    std::printf("S3 RIDGE  THE PATH IS LOST  stopped %d  stakes %d  score %d  crest %d\n", cart.stopped(),
                cart.stakes(), cart.score(), cart.crest() + 1);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 RIDGE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3ridge [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<ridge::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
