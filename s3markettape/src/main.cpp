// S3 MARKETTAPE
//   s3markettape                 stock the stall until the drawer matches the tape
//   s3markettape --sim           autopilot fills the drawer and leaves
//   s3markettape --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/tape.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    markettape::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (!cart.rules()) {
        std::printf("S3 MARKETTAPE  FAIL  rules  %s\n", cart.reason());
        std::fflush(stdout);
        return 1;
    }
    bool titled = false;
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 4) {
            save(sys, "title.png");
            titled = true;
        }
    }
    save(sys, "end.png");
    bool names = std::strcmp(cart.tapeLabel(0), "APPLE") == 0 && std::strcmp(cart.tapeLabel(1), "LOAF") == 0 &&
                 std::strcmp(cart.tapeLabel(2), "PEAR") == 0;
    bool scores = cart.tapeScore(0) == 12 && cart.tapeScore(1) == 9 && cart.tapeScore(2) == 7;
    if (cart.won() && cart.left() && cart.matched() && cart.rules() && names && scores && cart.drawerScore() == 28 &&
        cart.faults() == 0 && cart.heldLine(0) && cart.heldLine(1) && cart.heldLine(2)) {
        std::printf("S3 MARKETTAPE  PASS  the drawer matches the tape (%s, %s, %s) and you leave (%.1f s)\n",
                    cart.tapeLabel(0), cart.tapeLabel(1), cart.tapeLabel(2), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 MARKETTAPE  FAIL  %s  till %d  faults %d  lines %d%d%d  (%.1f s)\n", cart.reason(),
                cart.drawerScore(), cart.faults(), cart.heldLine(0) ? 1 : 0, cart.heldLine(1) ? 1 : 0,
                cart.heldLine(2) ? 1 : 0, frames / 60.0);
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
            std::printf("S3 MARKETTAPE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3markettape [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<markettape::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
