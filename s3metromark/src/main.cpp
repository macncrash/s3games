// S3 METRO MARK
//   s3metromark                 take the metro and set down on the mark
//   s3metromark --sim           autopilot sets down ahead of the other crew
//   s3metromark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/metro.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        metromark::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    metromark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool roll = false, close = false, down = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!roll && m >= 1 && frames > 40) {
            save(sys, "roll.png");
            roll = true;
        } else if (!close && m == 2) {
            save(sys, "close.png");
            close = true;
        } else if (!down && m == 3) {
            save(sys, "mark.png");
            down = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 METRO MARK  FAIL  %s  err %+.2f  (%.1f s)\n", why, cart.error(), cart.seconds());
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 METRO MARK  SET  set down on the mark ahead of the other crew  (%.1f s)\n", cart.seconds());
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 METRO MARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3metromark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<metromark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
