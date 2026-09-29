// S3 SHELVE MARK
//   s3shelvemark            play until a finished mark ends it
//   s3shelvemark --sim      autopilot holds the mark and leaves
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/mark.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    shelvemark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 45;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    const char* row = "ABCD";
    char letter = row[cart.markRow() & 3];
    if (cart.won() && cart.finished()) {
        std::printf("S3 SHELVE MARK  FINISHED MARK  row %c  shelved %d  back %d  (%.1f s)\n", letter, cart.shelved(),
                    cart.returned(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 SHELVE MARK  OPEN  no finished mark  row %c  shelved %d  back %d  (%.1f s)\n", letter,
                cart.shelved(), cart.returned(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SHELVE MARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3shelvemark [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<shelvemark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
