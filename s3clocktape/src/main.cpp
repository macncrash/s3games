// S3 CLOCKTAPE
//   s3clocktape                 play
//   s3clocktape --sim           the keeper files each line on the tape
//   s3clocktape --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/clock.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    clocktape::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 25;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.heldCount() == 1) {
            save(sys, "play.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    if (cart.rules() && cart.won() && cart.matched() && cart.faults() == 0 && cart.heldCount() == 3) {
        std::printf("S3 CLOCKTAPE  WIN  drawer matches the tape  4:00  7:30  10:15\n");
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 CLOCKTAPE  FAIL  drawer open  held %d  faults %d\n", cart.heldCount(), cart.faults());
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
            std::printf("S3 CLOCKTAPE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3clocktape [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<clocktape::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
