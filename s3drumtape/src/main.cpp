// S3 DRUMTAPE
//   s3drumtape                 strike the drum until the drawer matches the tape
//   s3drumtape --sim           autopilot files the tape and leaves
//   s3drumtape --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/drumtape.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    drumtape::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    save(sys, "boot.png");
    int frames = 0;
    const int limit = 60 * 40;
    bool drawer = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!drawer && cart.phase() == 2) {
            save(sys, "drawer.png");
            drawer = true;
        }
    }
    save(sys, "end.png");
    bool names = std::strcmp(cart.tapeLabel(0), "SHELL") == 0 && std::strcmp(cart.tapeLabel(1), "HEAD") == 0 &&
                 std::strcmp(cart.tapeLabel(2), "MALLET") == 0;
    bool scores = cart.tapeScore(0) == 6 && cart.tapeScore(1) == 9 && cart.tapeScore(2) == 3;
    if (cart.won() && cart.left() && cart.matched() && names && scores && cart.filled() == 3 &&
        cart.strikes() == 3 && cart.drawerScore() == 18 && cart.faults() == 0 && cart.held(0) && cart.held(1) &&
        cart.held(2)) {
        std::printf("S3 DRUMTAPE  PASS  the drawer matches the tape (%s, %s, %s) and you leave (%.1f s)\n",
                    cart.tapeLabel(0), cart.tapeLabel(1), cart.tapeLabel(2), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 DRUMTAPE  FAIL  %s  filled %d  till %d  faults %d  strikes %d  (%.1f s)\n", cart.reason(),
                cart.filled(), cart.drawerScore(), cart.faults(), cart.strikes(), frames / 60.0);
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
            std::printf("S3 DRUMTAPE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3drumtape [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<drumtape::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
