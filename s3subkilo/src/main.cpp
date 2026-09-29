// S3 SUB KILO
//   s3subkilo                 play
//   s3subkilo --sim           autopilot finishes the kilometer
//   s3subkilo --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/kilo.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        subkilo::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    subkilo::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool wheels = false, gate = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!wheels && m == 2) {
            save(sys, "wheels.png");
            wheels = true;
        } else if (!gate && m == 3) {
            save(sys, "kilo.png");
            gate = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 SUB KILO  FAIL  %s\n", cart.over() ? "the leg failed" : "timed out");
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 SUB KILO  WIN  the kilometer is finished without touching a wheel\n");
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
            std::printf("S3 SUB KILO %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3subkilo [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<subkilo::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
