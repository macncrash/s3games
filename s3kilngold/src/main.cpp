// S3 KILN GOLD
//   s3kilngold                 a short kiln; only the gold counts double
//   s3kilngold --sim           autopilot leaves on a gold double
//   s3kilngold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/kilngold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    kilngold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (frames == 8) save(sys, "title.png");
        if (frames == 80) save(sys, "kiln.png");
    }
    save(sys, "end.png");
    const int face = cart.golds() * kilngold::kGoldFace * 2 + cart.cream() * kilngold::kCreamFace;
    if (cart.won() && cart.finisherGold() && cart.golds() == kilngold::kGolds && cart.cream() == kilngold::kCreams &&
        cart.fires() == kilngold::kFires && cart.score() == face && cart.bare() < kilngold::kLine &&
        cart.score() >= kilngold::kLine) {
        std::printf(
            "S3 KILN GOLD  DOUBLE  only the gold counts double  score %d  bare %d  golds %d  cream %d  fires %d  "
            "(%.1f s)\n",
            cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.fires(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    const char* why = cart.over() ? cart.why() : "timed out";
    std::printf("S3 KILN GOLD  SHORT  %s  score %d  bare %d  golds %d  cream %d  fires %d  (%.1f s)\n", why,
                cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.fires(), frames / 60.0);
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
            std::printf("S3 KILN GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3kilngold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<kilngold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
