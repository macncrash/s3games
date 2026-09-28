// S3 CRANE LOCK
//   s3cranelock                 play
//   s3cranelock --sim           autopilot takes the crane through the lock
//   s3cranelock --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/lock.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        cranelock::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    cranelock::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    bool playShot = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!playShot && frames == 90) {
            save(sys, "lock.png");
            playShot = true;
        }
    }
    save(sys, cart.won() ? "clear.png" : "end.png");
    if (!cart.won()) {
        std::printf("S3 CRANE LOCK  FAIL  %s  (%.1f s)\n", cart.reason(), frames / 60.0);
        return 1;
    }
    std::printf("S3 CRANE LOCK  PASSED  took the crane through the lock ahead of the other crew  (%.1f s)\n",
                cart.seconds());
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 CRANE LOCK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3cranelock [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<cranelock::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
