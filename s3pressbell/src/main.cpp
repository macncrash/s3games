// S3 PRESS BELL
//   s3pressbell                 play
//   s3pressbell --sim           the bell rings before the third try dies
//   s3pressbell --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/pressbell.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    pressbell::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 30;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (frames == 6) save(sys, "title.png");
        if (!mid && cart.dead() == 2 && !cart.rung()) {
            save(sys, "press.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.rung() && cart.dead() == pressbell::kTries - 1 && cart.attempt() == pressbell::kTries - 1) {
        std::printf("S3 PRESS BELL  RUNG  the bell rings before the third try dies  dead %d  try %d  (%.1f s)\n",
                    cart.dead(), cart.attempt() + 1, frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    const char* why = cart.over() ? cart.why() : "timed out";
    std::printf("S3 PRESS BELL  SILENT  %s  dead %d  try %d  rung %d  (%.1f s)\n", why, cart.dead(), cart.attempt() + 1,
                cart.rung() ? 1 : 0, frames / 60.0);
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
            std::printf("S3 PRESS BELL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3pressbell [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<pressbell::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
