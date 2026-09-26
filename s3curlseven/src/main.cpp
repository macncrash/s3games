// S3 CURL SEVEN
//   s3curlseven                 play a short curl, first to seven
//   s3curlseven --sim           autopilot curls until you are first to seven
//   s3curlseven --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/seven.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        curlseven::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 16; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    curlseven::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    int slideSeen = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (cart.sliding() && ++slideSeen == 28) save(sys, "throw.png");
    }
    save(sys, "end.png");
    if (cart.rules() && cart.won() && cart.you() >= 7 && cart.them() < 7) {
        std::printf("S3 CURL SEVEN  PASS  first to seven  you %d  them %d  (%.1f s)\n", cart.you(), cart.them(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 CURL SEVEN  SHORT  you %d  them %d  rules %d  (%.1f s)\n", cart.you(), cart.them(),
                cart.rules() ? 1 : 0, frames / 60.0);
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
            std::printf("S3 CURL SEVEN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3curlseven [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<curlseven::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
