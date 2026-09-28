// S3 SHUFFLE TAPE
//   s3shuffletape                 slide until the drawer matches the tape
//   s3shuffletape --sim           autopilot fills the drawer and leaves
//   s3shuffletape --sim --shots D also writes PNGs into D
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
    shuffletape::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    save(sys, "boot.png");
    int frames = 0;
    const int limit = 60 * 25;
    bool slid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!slid && frames == 8) {
            save(sys, "slide.png");
            slid = true;
        }
    }
    save(sys, "end.png");
    bool names = std::strcmp(cart.tapeLabel(0), "3") == 0 && std::strcmp(cart.tapeLabel(1), "2") == 0 &&
                 std::strcmp(cart.tapeLabel(2), "1") == 0;
    bool held = std::strcmp(cart.drawerLabel(0), "3") == 0 && std::strcmp(cart.drawerLabel(1), "2") == 0 &&
                std::strcmp(cart.drawerLabel(2), "1") == 0;
    if (cart.won() && names && held) {
        std::printf("S3 SHUFFLE TAPE  PASS  the drawer matches the tape (%s, %s, %s) and you leave (%.1f s)\n",
                    cart.tapeLabel(0), cart.tapeLabel(1), cart.tapeLabel(2), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 SHUFFLE TAPE  FAIL  drawer %s %s %s  tape %s %s %s  (%.1f s)\n", cart.drawerLabel(0),
                cart.drawerLabel(1), cart.drawerLabel(2), cart.tapeLabel(0), cart.tapeLabel(1), cart.tapeLabel(2),
                frames / 60.0);
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
            std::printf("S3 SHUFFLE TAPE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3shuffletape [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<shuffletape::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
