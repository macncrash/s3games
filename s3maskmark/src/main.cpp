// S3 MASKMARK
//   s3maskmark                 cut the mask
//   s3maskmark --sim           autopilot finishes the mark
//   s3maskmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/maskmark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        maskmark::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    maskmark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool cutting = false;
    int frames = 0;
    const int limit = 60 * 25;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (cart.cuts() > 0 && !cutting) {
            save(sys, "bench.png");
            cutting = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.finished() && cart.sealed() && cart.cuts() == 4 &&
        !std::strcmp(cart.markName(), "finished")) {
        std::printf("S3 MASKMARK  FINISHED MARK  seal seated  cuts %d  (%.1f s)\n", cart.cuts(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 MASKMARK  OPEN  no finished mark  %s  cuts %d  (%.1f s)\n", cart.markName(), cart.cuts(),
                frames / 60.0);
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
            std::printf("S3 MASKMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3maskmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<maskmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
