// S3 SCOREMARK
//   s3scoremark                 play the score
//   s3scoremark --sim           autopilot finishes the mark
//   s3scoremark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/score.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    score::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    save(sys, "boot.png");
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (frames == 50) save(sys, "phrase.png");
        if (frames == 160) save(sys, "bar.png");
    }
    save(sys, "end.png");
    const char* verdict = cart.won() ? "PASS" : "FAIL";
    std::printf("S3 SCOREMARK  %s  finished mark  notes %d  tries %d  (%.1f s)\n", verdict, cart.struck(),
                cart.tries(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SCOREMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3scoremark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<score::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
