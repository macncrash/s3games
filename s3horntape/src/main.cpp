// S3 HORNTAPE
//   s3horntape                 play the horn until the drawer matches the tape
//   s3horntape --sim           autopilot files the tape and leaves
//   s3horntape --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/horntape.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    horntape::Game cart;
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
    bool names = std::strcmp(cart.tapeLabel(0), "LIP") == 0 && std::strcmp(cart.tapeLabel(1), "VALVE") == 0 &&
                 std::strcmp(cart.tapeLabel(2), "FLARE") == 0;
    bool scores = cart.tapeScore(0) == 5 && cart.tapeScore(1) == 8 && cart.tapeScore(2) == 6;
    if (cart.won() && cart.left() && cart.matched() && names && scores && cart.filled() == 3 && cart.blows() == 3 &&
        cart.drawerScore() == 19 && cart.faults() == 0 && cart.held(0) && cart.held(1) && cart.held(2)) {
        std::printf("S3 HORNTAPE  PASS  the drawer matches the tape (%s, %s, %s) and you leave (%.1f s)\n",
                    cart.tapeLabel(0), cart.tapeLabel(1), cart.tapeLabel(2), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 HORNTAPE  FAIL  %s  filled %d  till %d  faults %d  blows %d  (%.1f s)\n", cart.reason(),
                cart.filled(), cart.drawerScore(), cart.faults(), cart.blows(), frames / 60.0);
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
            std::printf("S3 HORNTAPE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3horntape [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<horntape::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
