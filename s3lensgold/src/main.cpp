// S3 LENS GOLD
//   s3lensgold                 play the lens until only the gold counts double
//   s3lensgold --sim           autopilot leaves on a gold double
//   s3lensgold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/lensgold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    lensgold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (frames == 8) save(sys, "title.png");
        if (frames == 40) save(sys, "lens.png");
    }
    save(sys, "end.png");
    const int face = cart.golds() * lensgold::kGoldFace * 2 + cart.cream() * lensgold::kCreamFace;
    if (cart.won() && cart.finisherGold() && cart.golds() == lensgold::kGolds && cart.cream() == lensgold::kCreams &&
        cart.plates() == lensgold::kPlates && cart.score() == face && cart.bare() < lensgold::kLine &&
        cart.score() >= lensgold::kLine) {
        std::printf(
            "S3 LENS GOLD  DOUBLE  only the gold counts double  score %d  bare %d  golds %d  cream %d  plates %d  "
            "(%.1f s)\n",
            cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.plates(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    const char* why = cart.over() ? cart.why() : "timed out";
    std::printf("S3 LENS GOLD  SHORT  %s  score %d  bare %d  golds %d  cream %d  plates %d  (%.1f s)\n", why,
                cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.plates(), frames / 60.0);
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
            std::printf("S3 LENS GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3lensgold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<lensgold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
