// S3 DART GOLD
//   s3dartgold                 501, only the gold counts double
//   s3dartgold --sim           autopilot leaves on a gold double
//   s3dartgold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/dartgold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        dartgold::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    dartgold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (!cart.rules()) {
        std::printf("S3 DART GOLD  SHORT  rules 0\n");
        std::fflush(stdout);
        return 1;
    }
    bool flying = false;
    int frames = 0;
    const int limit = 60 * 25;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!flying && frames == 30) {
            save(sys, "flight.png");
            flying = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.goldOut() && cart.left() == 0) {
        std::printf("S3 DART GOLD  DOUBLE  only the gold counts double  %s  cream %d  darts %d  (%.1f s)\n", cart.out(),
                    cart.cream(), cart.darts(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 DART GOLD  SHORT  left %d  hit %s  want %s  darts %d  (%.1f s)\n", cart.left(), cart.last(),
                cart.want(), cart.darts(), frames / 60.0);
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
            std::printf("S3 DART GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3dartgold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<dartgold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
