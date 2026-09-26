// S3 WICKETCHIME
//   s3wicketchime                 bowl the short wicket as the hour chimes
//   s3wicketchime --sim           autopilot, exits 0 only when the hour chimes
//   s3wicketchime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/wicketchime.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    wicketchime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (!cart.rules()) {
        std::printf("S3 WICKETCHIME  FAIL  RULES  0:00:00  ball 0  (0.0 s)\n");
        std::fflush(stdout);
        return 1;
    }
    bool titled = false, flown = false;
    int frames = 0;
    const int limit = 60 * 24;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 16) {
            save(sys, "title.png");
            titled = true;
        }
        if (!flown && cart.flying()) {
            save(sys, "bowl.png");
            flown = true;
        }
    }
    save(sys, "end.png");
    bool hour = cart.hour() == 12 && cart.minute() == 0 && cart.second() < wicketchime::kGraceSec;
    if (cart.won() && cart.wicket() && hour && cart.balls() >= 1 && cart.balls() <= wicketchime::kBalls &&
        std::strcmp(cart.reason(), "CHIME") == 0) {
        std::printf("S3 WICKETCHIME  WIN  the hour chimes  %d:%02d:%02d  ball %d  (%.1f s)\n", cart.hour(),
                    cart.minute(), cart.second(), cart.balls(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::fprintf(stderr, "s3wicketchime %s %s ball %d wicket %d\n", cart.phase(), cart.reason(), cart.balls(),
                 cart.wicket() ? 1 : 0);
    std::printf("S3 WICKETCHIME  FAIL  %s  %d:%02d:%02d  ball %d  (%.1f s)\n", cart.reason()[0] ? cart.reason() : "OPEN",
                cart.hour(), cart.minute(), cart.second(), cart.balls(), frames / 60.0);
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
            std::printf("S3 WICKETCHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3wicketchime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<wicketchime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
