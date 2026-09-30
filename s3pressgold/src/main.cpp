// S3 PRESS GOLD
//   s3pressgold                 a short press; only the gold counts double
//   s3pressgold --sim           autopilot leaves on a gold double
//   s3pressgold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/pressgold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    pressgold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (frames == 8) save(sys, "title.png");
        if (frames == 80) save(sys, "press.png");
    }
    save(sys, "end.png");
    const int face = cart.golds() * pressgold::kGoldFace * 2 + cart.cream() * pressgold::kCreamFace;
    if (cart.won() && cart.finisherGold() && cart.golds() == pressgold::kGolds && cart.cream() == pressgold::kCreams &&
        cart.pulls() == pressgold::kPulls && cart.score() == face && cart.bare() < pressgold::kLine &&
        cart.score() >= pressgold::kLine) {
        std::printf(
            "S3 PRESS GOLD  DOUBLE  only the gold counts double  score %d  bare %d  golds %d  cream %d  pulls %d  "
            "(%.1f s)\n",
            cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.pulls(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    const char* why = cart.over() ? cart.why() : "timed out";
    std::printf("S3 PRESS GOLD  SHORT  %s  score %d  bare %d  golds %d  cream %d  pulls %d  (%.1f s)\n", why,
                cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.pulls(), frames / 60.0);
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
            std::printf("S3 PRESS GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3pressgold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<pressgold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
