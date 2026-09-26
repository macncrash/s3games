// S3 PINSCHIME
//   s3pinschime                 bowl until the hour chimes
//   s3pinschime --sim           autopilot, exits 0 only when the hour chimes
//   s3pinschime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/pinschime.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    pinschime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, rolling = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 16) {
            save(sys, "title.png");
            titled = true;
        }
        if (!rolling && cart.rolling()) {
            save(sys, "roll.png");
            rolling = true;
        }
    }
    save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 PINSCHIME  WIN  the hour chimes  %d:%02d:%02d  pins %d  (%.1f s)\n", cart.hour(),
                    cart.minute(), cart.second(), cart.pinsDown(), frames / 60.0);
        return 0;
    }
    std::printf("S3 PINSCHIME  FAIL  %s  %d:%02d:%02d  pins %d  (%.1f s)\n", cart.reason(), cart.hour(),
                cart.minute(), cart.second(), cart.pinsDown(), frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 PINSCHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3pinschime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<pinschime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
