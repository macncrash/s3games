// S3 KEEP
//   s3keep                 hold the door
//   s3keep --sim           autopilot holds it for three minutes
//   s3keep --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/keep.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        keep::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 12; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    keep::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool door = false, end = false;
    int frames = 0;
    const int limit = 60 * 190;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (shotDir && !door && m == 1 && frames == 100) {
            save(sys, "door.png");
            door = true;
        } else if (shotDir && !end && (m == 2 || m == 3)) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (shotDir && !end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 KEEP  THE DOOR HELD FOR THREE MINUTES  score %d\n", cart.score());
        return 0;
    }
    std::printf("S3 KEEP  THE DOOR OPENS  score %d\n", cart.score());
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 KEEP %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3keep [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<keep::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
