// S3 MOSAIC BELL
//   s3mosaicbell                 three tries at the mosaic
//   s3mosaicbell --sim           autopilot rings the bell and leaves
//   s3mosaicbell --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/bell.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        mosaicbell::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 12; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    mosaicbell::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (!cart.rules()) {
        std::printf("S3 MOSAIC BELL  DEAD  rules failed  bell silent  (0.0 s)\n");
        std::fflush(stdout);
        return 1;
    }
    bool laid = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!laid && cart.tryNo() == 1 && frames > 20) {
            save(sys, "lay.png");
            laid = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.rung() && cart.deadTries() < 3 && cart.tryNo() >= 1 && cart.tryNo() <= 3) {
        std::printf("S3 MOSAIC BELL  BELL  left on try %d  before the third try died  (%.1f s)\n", cart.tryNo(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::fprintf(stderr, "s3mosaicbell %s\n", cart.reason());
    std::printf("S3 MOSAIC BELL  DEAD  third try died  bell silent  (%.1f s)\n", frames / 60.0);
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
            std::printf("S3 MOSAIC BELL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3mosaicbell [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<mosaicbell::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
