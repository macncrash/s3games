// S3 MARKET GOLD
//   s3marketgold                 play until only the gold counts double
//   s3marketgold --sim           autopilot leaves on a gold double
//   s3marketgold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/marketgold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    marketgold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, lined = false;
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames >= 16) {
            save(sys, "title.png");
            titled = true;
        }
        if (!lined && cart.served() >= 1) {
            save(sys, "line.png");
            lined = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.left() && cart.finisherGold() && cart.golds() > 0 && cart.score() == cart.dueSum() &&
        cart.bare() < cart.score() && cart.served() >= 8) {
        std::printf(
            "S3 MARKET GOLD  DOUBLE  only the gold counts double  score %d  bare %d  golds %d  cream %d  served %d  "
            "(%.1f s)\n",
            cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.served(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    const char* why = cart.over() ? cart.why() : "timed out";
    std::printf("S3 MARKET GOLD  SHORT  %s  score %d  bare %d  golds %d  cream %d  served %d  (%.1f s)\n", why,
                cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.served(), frames / 60.0);
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
            std::printf("S3 MARKET GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3marketgold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<marketgold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
