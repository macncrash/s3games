// S3 CUECHIME
//   s3cuechime                 a short cue; the hour has to chime
//   s3cuechime --sim           autopilot pots the ball as the hour strikes
//   s3cuechime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/cuechime.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    cuechime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, rolling = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
        if (!rolling && cart.travelling()) {
            save(sys, "cue.png");
            rolling = true;
        }
    }
    save(sys, "end.png");
    bool hour = cart.hour() == 12 && cart.minute() == 0 && cart.second() < 24;
    if (cart.won() && cart.potted() && hour && cart.strokes() >= 1 && std::strcmp(cart.reason(), "CHIME") == 0) {
        std::printf("S3 CUECHIME  WIN  the hour chimes  %d:%02d:%02d  cue %d  (%.1f s)\n", cart.hour(), cart.minute(),
                    cart.second(), cart.strokes(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 CUECHIME  FAIL  %s  %d:%02d:%02d  cue %d  (%.1f s)\n", cart.reason(), cart.hour(), cart.minute(),
                cart.second(), cart.strokes(), frames / 60.0);
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
            std::printf("S3 CUECHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3cuechime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<cuechime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
