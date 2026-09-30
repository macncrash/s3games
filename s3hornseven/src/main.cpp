// S3 HORNSEVEN
//   s3hornseven                 play the horn, first to seven, then leave
//   s3hornseven --sim           autopilot, exits 0 only when that is true
//   s3hornseven --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/horn.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    hornseven::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, called = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 12) {
            save(sys, "title.png");
            titled = true;
        }
        if (!called && cart.you() >= 1) {
            save(sys, "horn.png");
            called = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.you() >= hornseven::kGoal && cart.you() > cart.rival() &&
        std::strcmp(cart.reason(), "LEAVE") == 0) {
        std::printf("S3 HORNSEVEN  WIN  first to seven  you %d  rival %d  (%.1f s)\n", cart.you(), cart.rival(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 HORNSEVEN  FAIL  %s  you %d  rival %d  (%.1f s)\n", cart.reason()[0] ? cart.reason() : "OPEN",
                cart.you(), cart.rival(), frames / 60.0);
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
            std::printf("S3 HORNSEVEN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3hornseven [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<hornseven::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
