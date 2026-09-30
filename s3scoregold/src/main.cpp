// S3 SCORE GOLD
//   s3scoregold                 play until only the gold counts double
//   s3scoregold --sim           autopilot files on a gold double
//   s3scoregold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/scoregold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    scoregold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames >= 12) {
            save(sys, "title.png");
            titled = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.filed() && cart.finisherGold() && cart.golds() > 0 && cart.score() == cart.dueSum() &&
        cart.bare() < cart.score() && cart.lines() >= 6) {
        std::printf(
            "S3 SCORE GOLD  DOUBLE  only the gold counts double  score %d  bare %d  golds %d  cream %d  lines %d  "
            "(%.1f s)\n",
            cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.lines(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    const char* why = cart.over() ? cart.why() : "timed out";
    std::printf("S3 SCORE GOLD  SHORT  %s  score %d  bare %d  golds %d  cream %d  lines %d  (%.1f s)\n", why,
                cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.lines(), frames / 60.0);
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
            std::printf("S3 SCORE GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3scoregold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<scoregold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
