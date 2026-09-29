// S3 ANVIL GOLD
//   s3anvilgold                 a short anvil; only the gold counts double
//   s3anvilgold --sim           autopilot leaves on a gold double
//   s3anvilgold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/anvilgold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    anvilgold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (frames == 8) save(sys, "title.png");
        if (frames == 80) save(sys, "anvil.png");
    }
    save(sys, "end.png");
    const int face = cart.golds() * anvilgold::kGoldFace * 2 + cart.cream() * anvilgold::kCreamFace;
    if (cart.won() && cart.finisherGold() && cart.golds() == anvilgold::kGolds && cart.cream() == anvilgold::kCreams &&
        cart.heats() == anvilgold::kHeats && cart.score() == face && cart.bare() < anvilgold::kLine &&
        cart.score() >= anvilgold::kLine) {
        std::printf(
            "S3 ANVIL GOLD  DOUBLE  only the gold counts double  score %d  bare %d  golds %d  cream %d  heats %d  "
            "(%.1f s)\n",
            cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.heats(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    const char* why = cart.over() ? cart.why() : "timed out";
    std::printf("S3 ANVIL GOLD  SHORT  %s  score %d  bare %d  golds %d  cream %d  heats %d  (%.1f s)\n", why,
                cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.heats(), frames / 60.0);
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
            std::printf("S3 ANVIL GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3anvilgold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<anvilgold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
