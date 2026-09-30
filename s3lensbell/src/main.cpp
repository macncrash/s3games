// S3 LENSBELL
//   s3lensbell                 play the lens
//   s3lensbell --sim           autopilot rings the bell and leaves
//   s3lensbell --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/lensbell.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    lensbell::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    save(sys, "boot.png");
    int frames = 0;
    const int limit = 60 * 45;
    bool aimed = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (frames == 12) save(sys, "title.png");
        if (!aimed && cart.tryNo() == 1 && frames > 30) {
            save(sys, "aim.png");
            aimed = true;
        }
    }
    save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 LENSBELL  BELL  left on try %d  before the third try died  (%.1f s)\n", cart.tryNo(),
                    frames / 60.0);
        return 0;
    }
    std::printf("S3 LENSBELL  DEAD  third try died  bell silent  (%.1f s)\n", frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 LENSMBELL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3lensbell [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<lensbell::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
