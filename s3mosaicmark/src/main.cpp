// S3 MOSAIC MARK
//   s3mosaicmark                 play
//   s3mosaicmark --sim           autopilot lays the mark
//   s3mosaicmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/mark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    mark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool title = false, play = false, seal = false, end = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 0 && !title && frames > 8) {
            save(sys, "title.png");
            title = true;
        } else if (m == 1 && !play && frames > 20) {
            save(sys, "play.png");
            play = true;
        } else if (m == 2 && !seal) {
            save(sys, "seal.png");
            seal = true;
        } else if (m == 3 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (cart.won()) {
        std::printf("S3 MOSAIC MARK  PASS  mark finished  stamps %d  (%.1f s)\n", cart.stamps(), frames / 60.0);
        return 0;
    }
    std::printf("S3 MOSAIC MARK  FAIL  stamps %d  (%.1f s)\n", cart.stamps(), frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 MOSAIC MARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3mosaicmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<mark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
