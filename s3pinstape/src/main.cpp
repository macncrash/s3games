// S3 PINSTAPE
//   s3pinstape                 play a short rack
//   s3pinstape --sim           bowl until the drawer matches the tape
//   s3pinstape --sim --shots D also writes PNGs into D
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
    pinstape::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, rolling = false, matched = false;
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && cart.phase() == 0 && frames >= 10) {
            save(sys, "title.png");
            titled = true;
        }
        if (!rolling && cart.phase() == 2) {
            save(sys, "roll.png");
            rolling = true;
        }
        if (!matched && cart.phase() == 3) {
            save(sys, "drawer.png");
            matched = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 PINSTAPE  FAIL  line %d\n", cart.lineNo() + 1);
        std::fflush(stdout);
        return 1;
    }
    std::printf(
        "S3 PINSTAPE  PASS  the drawer matches the tape (%s, %s, %s) and the short rack is closed (%.1f s)\n",
        cart.tapeLabel(0), cart.tapeLabel(1), cart.tapeLabel(2), frames / 60.0);
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
            std::printf("S3 PINSTAPE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3pinstape [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<pinstape::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
