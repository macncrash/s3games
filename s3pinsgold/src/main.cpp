// S3 PINS GOLD
//   s3pinsgold                 bowl one frame
//   s3pinsgold --sim           autopilot, exits 0 only on a double
//   s3pinsgold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/pinsgold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    pinsgold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, rolling = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 20) {
            save(sys, "title.png");
            titled = true;
        }
        if (!rolling && frames == 80) {
            save(sys, "roll.png");
            rolling = true;
        }
    }
    save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 PINS GOLD  DOUBLE  score %d  gold %d  cream %d  (%.1f s)\n", cart.score(), cart.goldDown(),
                    cart.creamDown(), frames / 60.0);
        return 0;
    }
    std::printf("S3 PINS GOLD  SHORT  score %d  gold %d  cream %d  (%.1f s)\n", cart.score(), cart.goldDown(),
                cart.creamDown(), frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 PINS GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3pinsgold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<pinsgold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
