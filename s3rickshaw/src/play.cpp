// S3 RICKSHAW
//   s3rickshaw                 one fare, three turns
//   s3rickshaw --sim           autopilot must deliver without tipping
//   s3rickshaw --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "cartver.h"
#include "game/rickshaw.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        rickshaw::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 24; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    rickshaw::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false, bend = false;
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && frames == 150) {
            save(sys, "street.png");
            mid = true;
        }
        if (!bend && cart.travel() > 40.f && cart.travel() < 48.f) {
            save(sys, "bend.png");
            bend = true;
        }
    }
    if (shotDir) save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 RICKSHAW  FAIL  %s  s %.1f  x %.2f  psi %.2f  lean %.2f  v %.1f  (%.1f s)\n", cart.fail(),
                    cart.travel(), cart.offset(), cart.heading(), cart.lean(), cart.speed(), cart.seconds());
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 RICKSHAW  FARE PAID  Nina made three turns  the cab stayed up  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 RICKSHAW %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3rickshaw [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<rickshaw::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
