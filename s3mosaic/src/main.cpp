// S3 MOSAIC
//   s3mosaic                 play
//   s3mosaic --sim           autopilot slides until the picture is complete
//   s3mosaic --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/mosaic.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    mosaic::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool title = false, play = false, end = false;
    bool clear[mosaic::PICTURES] = {};
    int frames = 0;
    const int limit = 60 * 60;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 0 && !title && frames > 12) {
            save(sys, "title.png");
            title = true;
        } else if (m == 1 && !play) {
            save(sys, "play.png");
            play = true;
        } else if (m == 2) {
            int p = cart.picture();
            if (p >= 0 && p < mosaic::PICTURES && !clear[p]) {
                save(sys, std::string("picture-") + std::to_string(p + 1) + ".png");
                clear[p] = true;
            }
        } else if (m == 3 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (cart.won()) {
        std::printf("S3 MOSAIC  PASS  picture complete  pictures %d  moves %d  (%.1f s)\n", mosaic::PICTURES,
                    cart.moves(), frames / 60.0);
        return 0;
    }
    std::printf("S3 MOSAIC  FAIL  picture %d  moves %d  (%.1f s)\n", cart.picture() + 1, cart.moves(), frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 MOSAIC %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3mosaic [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<mosaic::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
