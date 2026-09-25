// S3 ORBIT
//   s3orbit                 dock to the arm
//   s3orbit --sim           autopilot soft-docks
//   s3orbit --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/orbit.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        orbit::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 10; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    orbit::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 110;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (shotDir && frames == 90) save(sys, "approach.png");
    }
    if (shotDir) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 ORBIT  SOFT DOCK  latched on the arm  contact %d\n", cart.contact());
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 ORBIT  FAIL  %s  (%.1f s)\n", cart.why(), frames / 60.0);
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
            std::printf("S3 ORBIT %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3orbit [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<orbit::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
