// S3 LOCK
//   s3lock                 steer the narrowboat through the lock
//   s3lock --sim           autopilot must pass without hitting a gate
//   s3lock --sim --shots D also writes PNGs into D
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
        s3lock::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    s3lock::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mouth = false, pound = false, upper = false;
    int frames = 0;
    const int limit = 60 * 80;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mouth && cart.phase() == 1 && cart.lowerOpen() > 0.45f) {
            save(sys, "mouth.png");
            mouth = true;
        } else if (!pound && cart.phase() == 3) {
            save(sys, "pound.png");
            pound = true;
        } else if (!upper && cart.phase() == 4 && cart.upperOpen() > 0.55f) {
            save(sys, "upper.png");
            upper = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 LOCK  FAIL  %s  x %.2f  hdg %.0f  spd %.2f  lo %.2f  hi %.2f  phase %d  (%.1f s)\n", why,
                    cart.lateral(), cart.heading() * 57.2958f, cart.speed(), cart.lowerOpen(), cart.upperOpen(),
                    cart.phase(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 LOCK  CLEAR  one lock, didn't hit the gates  (%.1f s)\n", frames / 60.0);
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
            std::printf("S3 LOCK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3lock [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<s3lock::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
