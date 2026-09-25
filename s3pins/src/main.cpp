// S3 PINS
//   s3pins                 play ten frames
//   s3pins --sim           autopilot bowls until a mark
//   s3pins --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/pins.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    pins::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, rolling = false;
    int frames = 0;
    const int limit = 60 * 45;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 24) {
            save(sys, "title.png");
            titled = true;
        }
        if (!rolling && frames == 90) {
            save(sys, "roll.png");
            rolling = true;
        }
    }
    save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 PINS  MARK  %s  frame %d  pins %d  (%.1f s)\n", cart.kind(), cart.frameNo(), cart.pinsDown(),
                    frames / 60.0);
        return 0;
    }
    std::printf("S3 PINS  OPEN  no mark  frame %d  pins %d  (%.1f s)\n", cart.frameNo(), cart.pinsDown(), frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 PINS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3pins [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<pins::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
