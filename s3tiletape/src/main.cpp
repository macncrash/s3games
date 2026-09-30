// S3 TILETAPE
//   s3tiletape                 set the short tiles until the drawer matches
//   s3tiletape --sim           autopilot matches the drawer and leaves
//   s3tiletape --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/tiletape.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    tiletape::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (!cart.rules()) {
        std::printf("S3 TILETAPE  FAIL  rules  %s\n", cart.reason());
        std::fflush(stdout);
        return 1;
    }
    bool title = false, play = false, seal = false, end = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 0 && !title && frames > 8) {
            save(sys, "title.png");
            title = true;
        } else if (m == 1 && !play && frames > 30) {
            save(sys, "play.png");
            play = true;
        } else if (m == 2 && !seal) {
            save(sys, "seal.png");
            seal = true;
        } else if (m == 3 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    save(sys, "end.png");
    bool order = true;
    for (int i = 0; i < tiletape::N; i++)
        if (cart.slot(i) != cart.tapeAt(i)) order = false;
    if (cart.won() && cart.left() && cart.matched() && cart.rules() && order && cart.holding() < 0 &&
        cart.moves() >= 3) {
        std::printf("S3 TILETAPE  PASS  the drawer matches the tape  moves %d  (%.1f s)\n", cart.moves(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 TILETAPE  FAIL  %s  moves %d  matched %d  (%.1f s)\n", cart.reason(), cart.moves(),
                cart.matched() ? 1 : 0, frames / 60.0);
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
            std::printf("S3 TILETAPE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3tiletape [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<tiletape::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
