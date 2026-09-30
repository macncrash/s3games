// S3 SALLY PACE
//   s3sallypace                 play
//   s3sallypace --sim           autopilot fires on the third pace
//   s3sallypace --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/sally.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        sally::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    sally::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool field = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!field && cart.pace() == 2) {
            save(sys, "pace.png");
            field = true;
        }
    }
    save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 SALLYPACE  PASS  fired on the third pace  (%.1f s)\n", frames / 60.0);
        return 0;
    }
    std::printf("S3 SALLYPACE  FAIL  %s  pace %d  (%.1f s)\n", cart.reason(), cart.pace(), frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SALLY PACE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3sallypace [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<sally::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
