// S3 QUARRY BANN
//   s3quarrybann                 play
//   s3quarrybann --sim           autopilot brings the banner back
//   s3quarrybann --sim --shots D also writes PNGs into D
#include "console/system.h"
#include "game/bann.h"
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
        qbann::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; ++i) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    qbann::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool haul = false, hand = false, end = false;
    int frames = 0, playFrames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        ++frames;
        int m = cart.marker();
        if (m == 1) {
            if (++playFrames == 50 && !haul) {
                save(sys, "haul.png");
                haul = true;
            }
        } else if (m == 2 && !hand) {
            save(sys, "banner.png");
            hand = true;
        } else if ((m == 3 || m == 4) && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (!cart.won() || !cart.carrying()) {
        std::printf("S3 QUARRY BANN  THE BANNER IS NOT BACK\n");
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 QUARRY BANN  THE BANNER IS BACK\n");
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
            std::printf("S3 QUARRY BANN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3quarrybann [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<qbann::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
