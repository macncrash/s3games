// S3 FISHTAPE
//   s3fishtape                 play fish until the drawer matches the tape
//   s3fishtape --sim           autopilot fills the drawer and leaves
//   s3fishtape --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/fishtape.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    fishtape::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (!cart.rules()) {
        std::printf("S3 FISHTAPE  FAIL  rules  %s\n", cart.reason());
        std::fflush(stdout);
        return 1;
    }
    bool titled = false, threw = false, drawer = false;
    int frames = 0;
    const int limit = 60 * 25;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 12) {
            save(sys, "title.png");
            titled = true;
        }
        if (!threw && cart.flying()) {
            save(sys, "shot.png");
            threw = true;
        }
        if (!drawer && cart.phase() == 3) {
            save(sys, "drawer.png");
            drawer = true;
        }
    }
    save(sys, "end.png");
    bool names = std::strcmp(cart.tapeLabel(0), "DAB") == 0 && std::strcmp(cart.tapeLabel(1), "BASS") == 0 &&
                 std::strcmp(cart.tapeLabel(2), "PIKE") == 0;
    bool scores = cart.tapeScore(0) == 1 && cart.tapeScore(1) == 3 && cart.tapeScore(2) == 6;
    if (cart.won() && cart.left() && cart.matched() && cart.rules() && names && scores && cart.casts() == 3 &&
        cart.drawerScore() == 10 && cart.board() == 10 && cart.traps() == 0 && cart.held(0) && cart.held(1) &&
        cart.held(2)) {
        std::printf("S3 FISHTAPE  PASS  the drawer matches the tape (%s, %s, %s) and you leave (%.1f s)\n",
                    cart.tapeLabel(0), cart.tapeLabel(1), cart.tapeLabel(2), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 FISHTAPE  FAIL  %s  %s  casts %d  till %d  board %d  traps %d  %d%d%d  (%.1f s)\n",
                cart.modeName(), cart.reason(), cart.casts(), cart.drawerScore(), cart.board(), cart.traps(),
                cart.held(0) ? 1 : 0, cart.held(1) ? 1 : 0, cart.held(2) ? 1 : 0, frames / 60.0);
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
            std::printf("S3 FISHTAPE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3fishtape [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<fishtape::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
