// S3 RIDGE CLER
//   s3ridgecler                 clear the ridge
//   s3ridgecler --sim           autopilot clears the ground before the clock
//   s3ridgecler --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/ridge.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        rcler::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 36; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    rcler::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool ridge = false, end = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!ridge && frames == 48) {
            save(sys, "ridge.png");
            ridge = true;
        }
        if (m == 3 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    const bool pass = cart.won() && cart.left() == 0;
    if (pass) {
        std::printf("S3 RIDGE CLER  THE GROUND IS CLEAR  the watch holds  (%.1f s)\n", frames / 60.0);
    } else {
        const char* why = cart.reason()[0] ? cart.reason() : "THE CLOCK DIED";
        std::printf("S3 RIDGE CLER  %s  left %d  (%.1f s)\n", why, cart.left(), frames / 60.0);
    }
    std::fflush(stdout);
    return pass ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 RIDGE CLER %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3ridgecler [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<rcler::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
