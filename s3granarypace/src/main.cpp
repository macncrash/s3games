// S3 GRANARYPACE
//   s3granarypace                 one granary; fire only after the third pace
//   s3granarypace --sim           autopilot waits, then fires
//   s3granarypace --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/granary.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    granary::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 20;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (frames == 6) save(sys, "title.png");
        if (!mid && cart.pace() == 2) {
            save(sys, "pace.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.pace() == granary::kPaces && cart.early() == 0 && cart.shots() == 1 &&
        std::strcmp(cart.reason(), "DONE") == 0) {
        std::printf("S3 GRANARYPACE  WIN  third pace then fire  pace %d  (%.1f s)\n", cart.pace(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 GRANARYPACE  FAIL  %s  pace %d  early %d  shots %d  (%.1f s)\n",
                cart.reason()[0] ? cart.reason() : "OPEN", cart.pace(), cart.early(), cart.shots(), frames / 60.0);
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
            std::printf("S3 GRANARYPACE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3granarypace [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<granary::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
