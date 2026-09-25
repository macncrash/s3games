// S3 ARCH
//   s3arch                 shoot the round
//   s3arch --sim           autopilot walks the dot into the gold
//   s3arch --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/arch.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    arch::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, aimed = false;
    int frames = 0;
    const int limit = 60 * 70;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!titled && m == 0 && frames >= 16) {
            save(sys, "title.png");
            titled = true;
        }
        if (!aimed && m == 1) {
            save(sys, "aim.png");
            aimed = true;
        }
    }
    save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 ARCH  WIN  score %d  golds %d  arrows %d  line %d  (%.1f s)\n", cart.score(), cart.golds(),
                    cart.shot(), cart.line(), frames / 60.0);
        return 0;
    }
    std::printf("S3 ARCH  SHORT  score %d  golds %d  arrows %d  line %d  (%.1f s)\n", cart.score(), cart.golds(),
                cart.shot(), cart.line(), frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 ARCH %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3arch [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<arch::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
