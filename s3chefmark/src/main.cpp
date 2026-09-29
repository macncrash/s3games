// S3 CHEFMARK
//   s3chefmark            plate the steak on the mark
//   s3chefmark --sim      autopilot finishes the mark
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/mark.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    chefmark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won() && cart.finished()) {
        std::printf("S3 CHEFMARK  FINISHED MARK  steak on the gold  score %d  (%.1f s)\n", cart.score(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    const char* why = cart.over() ? cart.fail() : "timed out";
    std::printf("S3 CHEFMARK  OPEN  no finished mark  %s  (%.1f s)\n", why, frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 CHEFMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3chefmark [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<chefmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
