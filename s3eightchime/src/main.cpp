// S3 EIGHTCHIME
//   s3eightchime                 pocket the 8 as the hour chimes
//   s3eightchime --sim           autopilot, exits 0 only when the hour chimes
//   s3eightchime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/eightchime.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    eightchime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (!cart.rules()) {
        std::printf("S3 EIGHTCHIME  FAIL  RULES  %d:%02d:%02d  stroke %d  (0.0 s)\n", cart.hour(), cart.minute(),
                    cart.second(), cart.strokes());
        std::fflush(stdout);
        return 1;
    }
    bool titled = false, rolling = false;
    int frames = 0;
    const int limit = 60 * 20;
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
    bool hour = cart.hour() == 12 && cart.minute() == 0 && cart.second() < 24;
    if (cart.won() && cart.pocketed() && hour && cart.strokes() >= 1 && cart.strokes() <= 3 &&
        std::strcmp(cart.reason(), "CHIME") == 0) {
        std::printf("S3 EIGHTCHIME  WIN  the hour chimes  %d:%02d:%02d  stroke %d  (%.1f s)\n", cart.hour(),
                    cart.minute(), cart.second(), cart.strokes(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    const char* why = cart.reason()[0] ? cart.reason() : "OPEN";
    std::printf("S3 EIGHTCHIME  FAIL  %s  %d:%02d:%02d  stroke %d  (%.1f s)\n", why, cart.hour(), cart.minute(),
                cart.second(), cart.strokes(), frames / 60.0);
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
            std::printf("S3 EIGHTCHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3eightchime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<eightchime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
