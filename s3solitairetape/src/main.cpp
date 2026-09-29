// S3 SOLITAIRE TAPE
//   s3solitairetape                 play until the drawer matches the tape
//   s3solitairetape --sim           autopilot files the tape and leaves
//   s3solitairetape --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/solitairetape.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    solitairetape::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (!cart.rules()) {
        std::printf("S3 SOLITAIRETAPE  DEAD  rules failed  drawer open  (0.0 s)\n");
        std::fflush(stdout);
        return 1;
    }
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (frames == 8) save(sys, "table.png");
    }
    save(sys, "end.png");
    bool names = std::strcmp(cart.tapeLabel(0), "ACE") == 0 && std::strcmp(cart.tapeLabel(1), "FIVE") == 0 &&
                 std::strcmp(cart.tapeLabel(2), "NINE") == 0;
    bool scores = cart.tapeScore(0) == 1 && cart.tapeScore(1) == 5 && cart.tapeScore(2) == 9;
    if (cart.won() && cart.left() && cart.matched() && names && scores && cart.faults() == 0 &&
        cart.drawerScore() == 15 && cart.held(0) && cart.held(1) && cart.held(2)) {
        std::printf("S3 SOLITAIRETAPE  PASS  the drawer matches the tape (%s, %s, %s) and you leave (%.1f s)\n",
                    cart.tapeLabel(0), cart.tapeLabel(1), cart.tapeLabel(2), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::fprintf(stderr, "s3solitairetape %s %s score %d faults %d held %d%d%d\n", cart.phase(), cart.reason(),
                 cart.drawerScore(), cart.faults(), cart.held(0) ? 1 : 0, cart.held(1) ? 1 : 0, cart.held(2) ? 1 : 0);
    std::printf("S3 SOLITAIRETAPE  DEAD  the drawer missed the tape  (%.1f s)\n", frames / 60.0);
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
            std::printf("S3 SOLITAIRE TAPE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3solitairetape [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<solitairetape::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
