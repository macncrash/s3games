// S3 DRUMMARK
//   s3drummark            play the short drum until the mark is finished
//   s3drummark --sim      autopilot lands the four strokes and leaves
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/drummark.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    drummark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won() && cart.finished()) {
        std::printf("S3 DRUMMARK  FINISHED MARK  hits %d/%d  misses %d  (%.1f s)\n", cart.hits(),
                    drummark::Game::kMarks, cart.misses(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 DRUMMARK  OPEN  no finished mark  hits %d/%d  misses %d  (%.1f s)\n", cart.hits(),
                drummark::Game::kMarks, cart.misses(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 DRUMMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3drummark [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<drummark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
