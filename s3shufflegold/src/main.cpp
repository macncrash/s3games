// S3 SHUFFLE GOLD
//   s3shufflegold                 one end; only the gold counts double
//   s3shufflegold --sim           autopilot, exits 0 only on that double
//   s3shufflegold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/gold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        shufflegold::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    shufflegold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool table = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!table && frames == 6) {
            save(sys, "table.png");
            table = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.goldZone() == 3 && cart.goldPts() == 6 && cart.creamZone() == 1 && cart.creamPts() == 1) {
        std::printf(
            "S3 SHUFFLE GOLD  DOUBLE  only the gold counts double  gold %d  cream %d  (%.1f s)\n",
            cart.goldPts(), cart.creamPts(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 SHUFFLE GOLD  OPEN  gold %d (zone %d)  cream %d (zone %d)  (%.1f s)\n", cart.goldPts(),
                cart.goldZone(), cart.creamPts(), cart.creamZone(), frames / 60.0);
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
            std::printf("S3 SHUFFLE GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3shufflegold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<shufflegold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
