// S3 RIDGE DOOR
//   s3ridgedoor                 play
//   s3ridgedoor --sim           autopilot holds the door
//   s3ridgedoor --sim --shots D also writes PNGs into D
#include "console/system.h"
#include "game/door.h"
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
        rdoor::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; ++i) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    rdoor::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool door = false, late = false, end = false;
    int frames = 0;
    const int limit = 60 * 200;
    while (!cart.over() && frames < limit) {
        sys.step();
        ++frames;
        int m = cart.marker();
        if (m == 1 && !door && frames > 420) {
            save(sys, "door.png");
            door = true;
        } else if (m == 3 && !late) {
            save(sys, "late.png");
            late = true;
        } else if (m == 4 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (!cart.won() || cart.watch() < 179.5f) {
        std::printf("S3 RIDGE DOOR  the watch is over  %s  misses %d  blocks %d  give %.2f  (%.1f s)\n", cart.reason(),
                    cart.misses(), cart.blocks(), cart.give(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 RIDGE DOOR  the door held for three minutes\n");
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 RIDGE DOOR %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3ridgedoor [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<rdoor::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
