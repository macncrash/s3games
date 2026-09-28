// S3 LANTERN GOLD
//   s3lanterngold                 light until only the gold counts double
//   s3lanterngold --sim           autopilot, exits 0 only on that double
//   s3lanterngold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/lanterngold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    lanterngold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, played = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
        if (!played && std::strcmp(cart.phase(), "play") == 0 && cart.chain() >= 2) {
            save(sys, "play.png");
            played = true;
        }
    }
    save(sys, "end.png");
    const bool math = cart.score() == cart.gold() * 2 + cart.cream();
    if (cart.won() && cart.finisherGold() && math && cart.gold() >= 1 && cart.score() >= cart.line() &&
        cart.bare() < cart.line()) {
        std::printf(
            "S3 LANTERN GOLD  DOUBLE  only the gold counts double  gold %d  cream %d  score %d  chain %d  (%.1f s)\n",
            cart.gold(), cart.cream(), cart.score(), cart.chain(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 LANTERN GOLD  NO DOUBLE  gold %d  cream %d  score %d  chain %d  %s  (%.1f s)\n", cart.gold(),
                cart.cream(), cart.score(), cart.chain(), cart.phase(), frames / 60.0);
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
            std::printf("S3 LANTERN GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3lanterngold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<lanterngold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
