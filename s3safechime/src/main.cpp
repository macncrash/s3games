// S3 SAFE CHIME
//   s3safechime                 dial the safe until the hour chimes
//   s3safechime --sim           autopilot, exits 0 only when the hour chimes
//   s3safechime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/chime.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        safechime::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    safechime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool dial = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!dial && cart.dialing() && cart.tumblers() > 0) {
            save(sys, "safe.png");
            dial = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.chimed() && cart.hour() == 12 && cart.minute() == 0 && cart.second() == 0 &&
        cart.tumblers() == safechime::kStops) {
        std::printf("S3 SAFECHIME  WIN  the hour chimes  %d:%02d:%02d  tumblers %d  (%.1f s)\n", cart.hour(),
                    cart.minute(), cart.second(), cart.tumblers(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 SAFECHIME  FAIL  %s  %d:%02d:%02d  tumblers %d  (%.1f s)\n", cart.reason(), cart.hour(),
                cart.minute(), cart.second(), cart.tumblers(), frames / 60.0);
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
            std::printf("S3 SAFE CHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3safechime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<safechime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
