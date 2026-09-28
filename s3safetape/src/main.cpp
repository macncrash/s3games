// S3 SAFETAPE
//   s3safetape                 play until the drawer matches the tape
//   s3safetape --sim           autopilot files the tape and leaves
//   s3safetape --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/safetape.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    safetape::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (!cart.rules()) {
        std::printf("S3 SAFETAPE  FAIL  rules  %s\n", cart.reason());
        std::fflush(stdout);
        return 1;
    }
    bool titled = false;
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 12) {
            save(sys, "title.png");
            titled = true;
        }
        if (cart.phase() == 2) save(sys, "drawer.png");
    }
    save(sys, "end.png");
    bool names = std::strcmp(cart.tapeLabel(0), "LEFT") == 0 && std::strcmp(cart.tapeLabel(1), "SPINE") == 0 &&
                 std::strcmp(cart.tapeLabel(2), "RIGHT") == 0;
    bool digits = cart.tapeDigit(0) == 4 && cart.tapeDigit(1) == 1 && cart.tapeDigit(2) == 8;
    bool near = cart.nearDigit(0) == 5 && cart.nearDigit(1) == 1 && cart.nearDigit(2) == 7;
    bool drawer = cart.drawerDigit(0) == 4 && cart.drawerDigit(1) == 1 && cart.drawerDigit(2) == 8;
    if (cart.won() && cart.left() && cart.matched() && cart.rules() && cart.nearRejected() && names && digits && near &&
        drawer && cart.held(0) && cart.held(1) && cart.held(2)) {
        std::printf("S3 SAFETAPE  PASS  the drawer matches the tape (%s, %s, %s) and you leave (%.1f s)\n",
                    cart.tapeLabel(0), cart.tapeLabel(1), cart.tapeLabel(2), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 SAFETAPE  FAIL  %s  drawer %d %d %d  near %d  (%.1f s)\n", cart.reason(), cart.drawerDigit(0),
                cart.drawerDigit(1), cart.drawerDigit(2), cart.nearRejected() ? 1 : 0, frames / 60.0);
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
            std::printf("S3 SAFETAPE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3safetape [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<safetape::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
