// S3 HOOP SEVEN
//   s3hoopseven                 play a short hoop, first to seven
//   s3hoopseven --sim           autopilot, exits 0 only on that seven
//   s3hoopseven --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/hoopseven.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    hoopseven::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, shot = false;
    int frames = 0, flyFrames = 0;
    const int limit = 60 * 60;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 16) {
            save(sys, "title.png");
            titled = true;
        }
        if (std::strcmp(cart.phase(), "flight") == 0) flyFrames++;
        else flyFrames = 0;
        if (!shot && flyFrames == 10) {
            save(sys, "shot.png");
            shot = true;
        }
    }
    save(sys, "end.png");
    const bool first = cart.won() && cart.you() >= 7 && cart.lane() < 7;
    if (first) {
        std::printf("S3 HOOP SEVEN  PASS  first to seven  you %d  lane %d  (%.1f s)\n", cart.you(), cart.lane(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 HOOP SEVEN  SHORT  you %d  lane %d  %s  (%.1f s)\n", cart.you(), cart.lane(), cart.phase(),
                frames / 60.0);
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
            std::printf("S3 HOOP SEVEN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3hoopseven [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<hoopseven::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
