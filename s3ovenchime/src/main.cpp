// S3 OVENCHIME
//   s3ovenchime                 play until the hour chimes
//   s3ovenchime --sim           autopilot, exits 0 only when the hour chimes
//   s3ovenchime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/ovenchime.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    ovenchime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, tended = false;
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 2) {
            save(sys, "title.png");
            titled = true;
        }
        if (!tended && cart.doorShut()) {
            save(sys, "oven.png");
            tended = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.hour() == 12 && cart.minute() == 0 && cart.second() == 0 && cart.doorShut() &&
        cart.inGold() && cart.strikes() < 3) {
        std::printf("S3 OVENCHIME  WIN  the hour chimes  %d:%02d:%02d  (%.1f s)\n", cart.hour(), cart.minute(),
                    cart.second(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::fprintf(stderr, "s3ovenchime %s strikes %d door %d gold %d %d:%02d:%02d\n",
                 cart.reason()[0] ? cart.reason() : "OPEN", cart.strikes(), cart.doorShut() ? 1 : 0,
                 cart.inGold() ? 1 : 0, cart.hour(), cart.minute(), cart.second());
    std::printf("S3 OVENCHIME  LOST  the hour did not chime  (%.1f s)\n", frames / 60.0);
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
            std::printf("S3 OVENCHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3ovenchime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<ovenchime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
