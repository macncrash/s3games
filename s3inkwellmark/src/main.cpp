// S3 INKWELLMARK
//   s3inkwellmark                 dip the quill, then the mark ends it
//   s3inkwellmark --sim           autopilot finishes the mark
//   s3inkwellmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/inkwellmark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    inkwellmark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false;
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
    }
    save(sys, "end.png");
    if (cart.rules() && cart.won() && cart.finished() && cart.inked() && cart.strokes() == 4 &&
        cart.dips() == 4) {
        std::printf("S3 INKWELLMARK  FINISHED MARK  ink seated  strokes %d  (%.1f s)\n", cart.strokes(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 INKWELLMARK  OPEN  no finished mark  strokes %d  dips %d  (%.1f s)\n", cart.strokes(),
                cart.dips(), frames / 60.0);
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
            std::printf("S3 INKWELLMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3inkwellmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<inkwellmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
