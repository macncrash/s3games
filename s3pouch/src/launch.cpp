// S3 POUCH
//   s3pouch                 play
//   s3pouch --sim           autopilot carries the pouch across three streets
//   s3pouch --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/pouch.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        pouch::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    pouch::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && frames == 70) {
            save(sys, "cross.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 POUCH  PASS  pouch across three streets  (%.1f s)\n", cart.seconds());
        return 0;
    }
    std::printf("S3 POUCH  FAIL  %s  crossed %d  (%.1f s)\n", cart.held() ? "still in the street" : "pouch dropped",
                cart.crossed(), cart.seconds());
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 POUCH %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3pouch [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<pouch::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
