// S3 TILEMARK
//   s3tilemark            play tile until the mark is finished
//   s3tilemark --sim      autopilot lays the row and leaves
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/tilemark.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    tilemark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won() && cart.finished() && cart.marked() && cart.laid() == tilemark::Game::kN) {
        std::printf("S3 TILEMARK  FINISHED MARK  laid %d/%d  (%.1f s)\n", cart.laid(), tilemark::Game::kN,
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 TILEMARK  OPEN  no finished mark  laid %d/%d  (%.1f s)\n", cart.laid(), tilemark::Game::kN,
                frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 TILEMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3tilemark [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<tilemark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
