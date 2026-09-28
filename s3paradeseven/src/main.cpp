// S3 PARADE SEVEN
//   s3paradeseven                 play parade until first to seven
//   s3paradeseven --sim           autopilot marches until that is true, then leaves
//   s3paradeseven --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/parade.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    paradeseven::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, mid = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
        if (!mid && cart.you() >= 3) {
            save(sys, "march.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.you() >= 7 && cart.you() > cart.them()) {
        std::printf("S3 PARADE SEVEN  PASS  first to seven  you %d  them %d  rows %d  (%.1f s)\n", cart.you(),
                    cart.them(), cart.rows(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 PARADE SEVEN  FAIL  you %d  them %d  rows %d  phase %s  (%.1f s)\n", cart.you(), cart.them(),
                cart.rows(), cart.phase(), frames / 60.0);
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
            std::printf("S3 PARADE SEVEN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3paradeseven [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<paradeseven::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
