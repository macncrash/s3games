// S3 GATE CLER
//   s3gatecler                 clear the yard
//   s3gatecler --sim           autopilot clears the ground before the clock
//   s3gatecler --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/cler.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        cler::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    cler::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool yard = false, end = false;
    int frames = 0;
    const int limit = 60 * 70;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!yard && frames == 70) {
            save(sys, "yard.png");
            yard = true;
        }
        if (m == 3 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    const char* why = cart.reason()[0] ? cart.reason() : "THE CLOCK DIED";
    if (cart.won() && cart.left() == 0) {
        std::printf("S3 GATE CLER  %s  (%.1f s)\n", why, frames / 60.0);
    } else {
        std::printf("S3 GATE CLER  %s  left %d  (%.1f s)\n", why, cart.left(), frames / 60.0);
    }
    std::fflush(stdout);
    return cart.won() && cart.left() == 0 ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 GATE CLER %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3gatecler [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<cler::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
