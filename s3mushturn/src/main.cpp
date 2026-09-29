// S3 MUSHTURN
//   s3mushturn            play
//   s3mushturn --sim      the team makes the three turns
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/turn.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    mushturn::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won())
        std::printf("S3 MUSHTURN  WIN  three turns made  (%.1f s)\n", frames / 60.0);
    else
        std::printf("S3 MUSHTURN  FAIL  turns %d  (%.1f s)\n", cart.turns(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 MUSHTURN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3mushturn [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<mushturn::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
