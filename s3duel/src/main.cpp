// S3 DUEL
//   s3duel                 three paces, then draw
//   s3duel --sim           autopilot waits, then fires clean
//   s3duel --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/duel.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    duel::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, paced = false, held = false, drew = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 12) {
            save(sys, "title.png");
            titled = true;
        }
        if (!paced && cart.phase() == 2 && cart.pace() == 2) {
            save(sys, "pace.png");
            paced = true;
        }
        if (!held && cart.phase() == 4) {
            save(sys, "hold.png");
            held = true;
        }
        if (!drew && cart.phase() == 5) {
            save(sys, "draw.png");
            drew = true;
        }
    }
    save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 DUEL  WON  three paces, clean draw  (%.1f s)\n", frames / 60.0);
        return 0;
    }
    std::printf("S3 DUEL  LOST  %s  (%.1f s)\n", cart.reason(), frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 DUEL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3duel [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<duel::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
