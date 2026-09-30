// S3 SCORETAPE
//   s3scoretape                 play a short score
//   s3scoretape --sim           file until the drawer matches the tape
//   s3scoretape --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/scoretape.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    scoretape::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (frames == 8) save(sys, "title.png");
        if (frames == 40) save(sys, "staff.png");
        if (cart.phase() == 2 && frames > 8) save(sys, "drawer.png");
    }
    save(sys, "end.png");
    if (!cart.won() || !cart.left() || !cart.matched() || cart.drawerScore() != 16) {
        std::printf("S3 SCORETAPE  FAIL  %s  till %d\n", cart.reason(), cart.drawerScore());
        std::fflush(stdout);
        return 1;
    }
    std::printf(
        "S3 SCORETAPE  PASS  the drawer matches the tape (%s %d, %s %d, %s %d) and you leave (%.1f s)\n",
        cart.tapeLabel(0), cart.tapeScore(0), cart.tapeLabel(1), cart.tapeScore(1), cart.tapeLabel(2),
        cart.tapeScore(2), frames / 60.0);
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SCORETAPE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3scoretape [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<scoretape::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
