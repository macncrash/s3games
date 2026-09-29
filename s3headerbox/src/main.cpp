// S3 HEADER BOX
//   s3headerbox                 sail the header, stop inside the box
//   s3headerbox --sim           autopilot must stop inside the box
//   s3headerbox --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/header.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    {
        gs::System sys(true);
        headerbox::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    headerbox::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 45;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (shotDir && frames == 50) save(sys, "sail.png");
        if (shotDir && !mid && frames > 120) {
            save(sys, "box.png");
            mid = true;
        }
    }
    if (shotDir) save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 HEADER BOX  FAIL  (%.1f s)\n", frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 HEADER BOX  WIN  stopped inside the box  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 HEADER BOX %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3headerbox [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<headerbox::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
