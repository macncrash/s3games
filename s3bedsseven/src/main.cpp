// S3 BEDS SEVEN
//   s3bedsseven                 water beds until someone reaches seven
//   s3bedsseven --sim           autopilot is first to seven
//   s3bedsseven --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/bedsseven.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        bedsseven::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 20; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    bedsseven::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    bool mid = false;
    const int limit = 60 * 45;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.marker() == 1 && frames == 70) {
            save(sys, "beds.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.you() >= 7 && cart.you() > cart.them()) {
        std::printf("S3 BEDS SEVEN  WIN  first to seven  you %d  them %d  (%.1f s)\n", cart.you(), cart.them(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 BEDS SEVEN  SHORT  you %d  them %d  (%.1f s)\n", cart.you(), cart.them(), frames / 60.0);
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
            std::printf("S3 BEDS SEVEN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3bedsseven [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<bedsseven::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
