// S3 MAZECHIME
//   s3mazechime                 walk until the hour chimes, then leave
//   s3mazechime --sim           autopilot, exits 0 only when the hour chimes
//   s3mazechime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/mazechime.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        mazechime::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 12; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    mazechime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (!cart.rules()) {
        std::printf("S3 MAZECHIME  FAIL  RULES  0:00:00  step 0  (0.0 s)\n");
        std::fflush(stdout);
        return 1;
    }
    bool walked = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!walked && cart.walking()) {
            save(sys, "walk.png");
            walked = true;
        }
    }
    save(sys, "end.png");
    bool hour = cart.hour() == 12 && cart.minute() == 0 && cart.second() < mazechime::kGraceSec;
    if (cart.won() && hour && cart.onGate() && cart.steps() >= 8 && cart.steps() <= 40 &&
        std::strcmp(cart.reason(), "CHIME") == 0) {
        std::printf("S3 MAZECHIME  WIN  the hour chimes  %d:%02d:%02d  step %d  (%.1f s)\n", cart.hour(),
                    cart.minute(), cart.second(), cart.steps(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 MAZECHIME  FAIL  %s  %d:%02d:%02d  step %d  (%.1f s)\n", cart.reason()[0] ? cart.reason() : "OPEN",
                cart.hour(), cart.minute(), cart.second(), cart.steps(), frames / 60.0);
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
            std::printf("S3 MAZECHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3mazechime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<mazechime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
