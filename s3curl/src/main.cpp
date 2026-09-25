// S3 CURL
//   s3curl            play four ends
//   s3curl --sim      autopilot must take the match
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/curl.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    curl::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 150;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (!cart.over() || !cart.won() || cart.red() <= cart.yel() || cart.ends() != 4) {
        std::printf("S3 CURL  FAIL  %d-%d  ends %d  (%.1f s)\n", cart.red(), cart.yel(), cart.ends(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 CURL  RED TAKES THE MATCH  %d-%d  four ends  (%.1f s)\n", cart.red(), cart.yel(), frames / 60.0);
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 CURL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3curl [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<curl::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
