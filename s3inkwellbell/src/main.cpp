// S3 INKWELL BELL
//   s3inkwellbell                 catch ink until the bell rings
//   s3inkwellbell --sim           autopilot leaves once that is true
//   s3inkwellbell --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/bell.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        inkwellbell::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    inkwellbell::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    bool playShot = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!playShot && frames == 40) {
            save(sys, "inkwell.png");
            playShot = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.left() && cart.marks() >= inkwellbell::kMarks && cart.tries() > 0) {
        std::printf("S3 INKWELL BELL  WIN  bell  marks %d  tries %d  (%.1f s)\n", cart.marks(), cart.tries(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 INKWELL BELL  SHORT  marks %d  tries %d  (%.1f s)\n", cart.marks(), cart.tries(), frames / 60.0);
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
            std::printf("S3 INKWELL BELL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3inkwellbell [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<inkwellbell::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
