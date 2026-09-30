// S3 QUILLTAPE
//   s3quilltape                 play quill until the drawer matches the tape
//   s3quilltape --sim           autopilot, exits 0 only when that is true
//   s3quilltape --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/quilltape.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    quilltape::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (!cart.rules()) {
        std::printf("S3 QUILLTAPE  FAIL  rules  %s\n", cart.reason());
        std::fflush(stdout);
        return 1;
    }
    bool titled = false, wrote = false, drawer = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 12) {
            save(sys, "title.png");
            titled = true;
        }
        if (!wrote && cart.writing()) {
            save(sys, "stroke.png");
            wrote = true;
        }
        if (!drawer && cart.phase() == 3) {
            save(sys, "drawer.png");
            drawer = true;
        }
    }
    save(sys, "end.png");
    const bool names = std::strcmp(cart.tapeLabel(0), "HAIR") == 0 && std::strcmp(cart.tapeLabel(1), "GOTH") == 0 &&
                       std::strcmp(cart.tapeLabel(2), "SWASH") == 0;
    const bool scores = cart.tapeScore(0) == 1 && cart.tapeScore(1) == 2 && cart.tapeScore(2) == 3;
    if (cart.won() && cart.left() && cart.matched() && cart.rules() && names && scores && cart.strokes() == 3 &&
        cart.drawerScore() == 6 && cart.board() == 6 && cart.traps() == 0 && cart.held(0) && cart.held(1) &&
        cart.held(2)) {
        std::printf(
            "S3 QUILLTAPE  PASS  the drawer matches the tape (%s, %s, %s) and the short quill is closed (%.1f s)\n",
            cart.tapeLabel(0), cart.tapeLabel(1), cart.tapeLabel(2), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 QUILLTAPE  FAIL  %s  strokes %d  till %d  board %d  traps %d  %d%d%d  (%.1f s)\n", cart.say(),
                cart.strokes(), cart.drawerScore(), cart.board(), cart.traps(), cart.held(0) ? 1 : 0,
                cart.held(1) ? 1 : 0, cart.held(2) ? 1 : 0, frames / 60.0);
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
            std::printf("S3 QUILLTAPE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3quilltape [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<quilltape::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
