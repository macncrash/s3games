// S3 BELLTAPE
//   s3belltape                 play the bell until the drawer matches the tape
//   s3belltape --sim           autopilot files the tape and leaves
//   s3belltape --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/belltape.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    belltape::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (!cart.rules()) {
        std::printf("S3 BELLTAPE  FAIL  rules  %s\n", cart.reason());
        std::fflush(stdout);
        return 1;
    }
    bool titled = false;
    bool drawer = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
        if (!drawer && cart.phase() == 2) {
            save(sys, "drawer.png");
            drawer = true;
        }
    }
    save(sys, "end.png");
    bool names = std::strcmp(cart.tapeLabel(0), "PEAL") == 0 && std::strcmp(cart.tapeLabel(1), "KNOCK") == 0 &&
                 std::strcmp(cart.tapeLabel(2), "TOLL") == 0;
    bool scores = cart.tapeScore(0) == 6 && cart.tapeScore(1) == 4 && cart.tapeScore(2) == 5;
    if (cart.won() && cart.left() && cart.matched() && cart.rules() && names && scores && cart.filled() == 3 &&
        cart.rings() == 3 && cart.drawerScore() == 15 && cart.faults() == 0 && cart.held(0) && cart.held(1) &&
        cart.held(2)) {
        std::printf("S3 BELLTAPE  PASS  the drawer matches the tape (%s, %s, %s) and you leave (%.1f s)\n",
                    cart.tapeLabel(0), cart.tapeLabel(1), cart.tapeLabel(2), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 BELLTAPE  FAIL  filled %d  score %d  faults %d  rings %d  held %d%d%d  %s  (%.1f s)\n",
                cart.filled(), cart.drawerScore(), cart.faults(), cart.rings(), cart.held(0) ? 1 : 0,
                cart.held(1) ? 1 : 0, cart.held(2) ? 1 : 0, cart.reason(), frames / 60.0);
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
            std::printf("S3 BELLTAPE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3belltape [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<belltape::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
