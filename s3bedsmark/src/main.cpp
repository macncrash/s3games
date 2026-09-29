// S3 BEDSMARK
//   s3bedsmark                 water the marked bed, then lift the stake
//   s3bedsmark --sim           autopilot finishes the mark
//   s3bedsmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/bedsmark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    bedsmark::Game cart;
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
    if (cart.won() && cart.finished() && cart.lifted() && cart.opened() && cart.markBed() >= 0 &&
        cart.wetMark() >= 1.f) {
        std::printf("S3 BEDSMARK  FINISHED MARK  bed %d lifted  (%.1f s)\n", cart.markBed() + 1, frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 BEDSMARK  OPEN  no finished mark  wet %.2f  (%.1f s)\n", cart.wetMark(), frames / 60.0);
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
            std::printf("S3 BEDSMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3bedsmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<bedsmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
