// S3 DRAWER GOLD
//   s3drawergold                 file until only the gold counts double
//   s3drawergold --sim           autopilot leaves on a gold double
//   s3drawergold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/drawergold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    drawergold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false;
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 2) {
            save(sys, "title.png");
            titled = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.goldOut() && cart.score() >= cart.line() && cart.bare() < cart.line() && cart.golds() > 0 &&
        cart.cream() == 0) {
        std::printf(
            "S3 DRAWER GOLD  DOUBLE  only the gold counts double  score %d  bare %d  golds %d  cream %d  filed %d  "
            "line %d  (%.1f s)\n",
            cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.filed(), cart.line(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 DRAWER GOLD  SHORT  score %d  bare %d  golds %d  cream %d  filed %d  line %d  (%.1f s)\n",
                cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.filed(), cart.line(), frames / 60.0);
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
            std::printf("S3 DRAWER GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3drawergold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<drawergold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
