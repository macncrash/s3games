// S3 DART SEVEN
//   s3dartseven                 play a race, first to seven
//   s3dartseven --sim           autopilot throws until you are first to seven
//   s3dartseven --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/dartseven.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        dartseven::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 20; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    dartseven::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool flying = false;
    int frames = 0;
    const int limit = 60 * 25;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!flying && cart.you() + cart.them() == 0 && frames > 30) {
            save(sys, "throw.png");
            flying = true;
        }
    }
    save(sys, "end.png");
    if (cart.rules() && cart.won() && cart.you() >= 7 && cart.them() < 7) {
        std::printf("S3 DART SEVEN  PASS  first to seven  you %d  them %d  (%.1f s)\n", cart.you(), cart.them(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 DART SEVEN  SHORT  you %d  them %d  rules %d  (%.1f s)\n", cart.you(), cart.them(),
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
            std::printf("S3 DART SEVEN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3dartseven [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<dartseven::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
