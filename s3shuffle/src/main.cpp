// S3 SHUFFLE
//   s3shuffle                 play
//   s3shuffle --sim           autopilot plays first to fifteen
//   s3shuffle --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/shuffle.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        shuffle::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 20; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    shuffle::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool table = false;
    int frames = 0;
    const int limit = 60 * 45;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!table && cart.frames() >= 1) {
            save(sys, "table.png");
            table = true;
        }
    }
    if (shotDir) save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 SHUFFLE  FAIL  you %d  house %d  frames %d  %s  (%.1f s)\n", cart.you(), cart.house(),
                    cart.frames(), cart.dump().c_str(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 SHUFFLE  PASS  first to fifteen  you %d  house %d  frames %d  (%.1f s)\n", cart.you(),
                cart.house(), cart.frames(), frames / 60.0);
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SHUFFLE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3shuffle [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<shuffle::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
