// S3 KILN SEVEN
//   s3kilnseven                 play the short kiln, first to seven
//   s3kilnseven --sim           autopilot fires until you are first to seven
//   s3kilnseven --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/kilnseven.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    kilnseven::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, kiln = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
        if (!kiln && std::strcmp(cart.phase(), "heat") == 0 && cart.you() == 0) {
            save(sys, "kiln.png");
            kiln = true;
        }
    }
    save(sys, "end.png");
    if (cart.rules() && cart.won() && cart.shortSix() && cart.you() >= 7 && cart.them() < 7) {
        std::printf("S3 KILN SEVEN  PASS  first to seven  you %d  them %d  (%.1f s)\n", cart.you(), cart.them(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 KILN SEVEN  SHORT  you %d  them %d  rules %d  six %d  phase %s  (%.1f s)\n", cart.you(),
                cart.them(), cart.rules() ? 1 : 0, cart.shortSix() ? 1 : 0, cart.phase(), frames / 60.0);
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
            std::printf("S3 KILN SEVEN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3kilnseven [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<kilnseven::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
