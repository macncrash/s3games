// S3 WHARF CLER
//   s3wharfcler                 clear the wharf ground
//   s3wharfcler --sim           autopilot clears the ground before the clock
//   s3wharfcler --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/wharf.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        wcler::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 24; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    wcler::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 50;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.left() <= 3) {
            save(sys, "wharf.png");
            mid = true;
        }
    }
    save(sys, cart.won() ? "clear.png" : "end.png");
    if (cart.won() && cart.left() == 0) {
        std::printf("S3 WHARF CLER  WIN  the ground is clear  (%.1f s)\n", frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    const char* why = cart.reason()[0] ? cart.reason() : "THE WATCH IS OVER";
    std::printf("S3 WHARF CLER  FAIL  %s  left %d  (%.1f s)\n", why, cart.left(), frames / 60.0);
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
            std::printf("S3 WHARF CLER %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3wharfcler [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<wcler::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
