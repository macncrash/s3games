// S3 GRANARY DOOR
//   s3granarydoor            play
//   s3granarydoor --sim      hold the door until the watch is won
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/door.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    granary::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 200;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    const int sec = cart.held() / 60;
    std::printf("S3 GRANARY DOOR  %s  door %d  held %d:%02d  (%.1f s)\n", cart.won() ? "PASS" : "FAIL", cart.door(),
                sec / 60, sec % 60, frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 GRANARY DOOR %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3granarydoor [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<granary::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
