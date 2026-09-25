// S3 GOLF
//   s3golf            play three holes
//   s3golf --sim      autopilot putts every ball into the cup
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/golf.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    golf::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    const bool won = cart.won() && cart.cups() == 3;
    if (!won) {
        std::printf("S3 GOLF  FAIL  cups %d/3  hole %d  strokes %d  lie %s  ball %.1f  (%.1f s)\n", cart.cups(),
                    cart.holeNo(), cart.strokes(), cart.lie(), cart.ballX(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 GOLF  IN THE HOLE  cups %d/3  strokes %d  (%.1f s)\n", cart.cups(), cart.strokes(),
                frames / 60.0);
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 GOLF %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3golf [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<golf::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
