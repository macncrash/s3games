// S3 BELL BELL
//   s3bellbell                 strike until the bell rings
//   s3bellbell --sim           autopilot, exits 0 only when that bell has rung
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/bell.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    bellbell::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won() && cart.rung() && cart.deadTries() < 3 && cart.tryNo() >= 1) {
        std::printf("S3 BELL BELL  RUNG  the bell rings before the third try dies  dead %d  try %d  (%.1f s)\n",
                    cart.deadTries(), cart.tryNo(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 BELL BELL  OPEN  dead %d  try %d  %s  (%.1f s)\n", cart.deadTries(), cart.tryNo(), cart.reason(),
                frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 BELL BELL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3bellbell [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<bellbell::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
