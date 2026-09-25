// S3 BOCCE
//   s3bocce                 play
//   s3bocce --sim           autopilot bowls to seven
//   s3bocce --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/bocce.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        bocce::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 12; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    bocce::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool court = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!court && cart.ends() >= 1) {
            save(sys, "court.png");
            court = true;
        }
    }
    if (shotDir) save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 BOCCE  FAIL  you %d  them %d  ends %d  %s  (%.1f s)\n", cart.you(), cart.them(), cart.ends(),
                    cart.dump().c_str(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 BOCCE  PASS  first to seven  you %d  them %d  ends %d  (%.1f s)\n", cart.you(), cart.them(),
                cart.ends(), frames / 60.0);
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
            std::printf("S3 BOCCE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3bocce [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<bocce::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
