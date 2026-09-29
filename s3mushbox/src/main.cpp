// S3 MUSHBOX
//   s3mushbox                 play
//   s3mushbox --sim           autopilot stops the mush in every box
//   s3mushbox --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/mush.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        mushbox::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 20; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    mushbox::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool slide = false, boxed = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!slide && m == 1) {
            save(sys, "slide.png");
            slide = true;
        } else if (!boxed && m == 2) {
            save(sys, "box.png");
            boxed = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won() || cart.boxes() != 3) {
        std::printf("S3 MUSHBOX  FAIL  boxes %d/3  stage %d  (%.1f s)\n", cart.boxes(), cart.stage() + 1,
                    frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 MUSHBOX  PASS  stopped in the box  %d/3  (%.1f s)\n", cart.boxes(), frames / 60.0);
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
            std::printf("S3 MUSHBOX %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3mushbox [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<mushbox::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
