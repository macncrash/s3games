// S3 CABLE
//   s3cable                 drive the hill tram
//   s3cable --sim           autopilot stops level with every mark
//   s3cable --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/cable.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    cable::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);

    bool title = false, climb = false, end = false;
    int frames = 0;
    const int limit = 60 * 100;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 0 && !title && frames > 6) {
            save(sys, "title.png");
            title = true;
        } else if (m == 2 && !climb && cart.cleared() >= 1) {
            save(sys, "mark.png");
            climb = true;
        } else if (m == 4 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (!cart.over()) {
        std::printf("S3 CABLE  FAIL  the hill did not end\n");
        return 1;
    }
    std::printf("%s\n", cart.report());
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 CABLE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3cable [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<cable::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
