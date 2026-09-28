// S3 PARADE CHIME
//   s3paradechime                 march until the hour has to chime
//   s3paradechime --sim           autopilot, exits 0 only when the hour chimes
//   s3paradechime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/chime.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    paradechime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
    }
    save(sys, "end.png");
    bool hour = cart.chimed() && cart.hour() == 12 && cart.minute() == 0 && cart.second() == 0;
    if (cart.won() && hour && std::strcmp(cart.reason(), "CHIME") == 0 && cart.lives() > 0 && cart.tries() >= 1) {
        std::printf("S3 PARADE CHIME  WIN  the hour chimes  %d:%02d:%02d  try %d  (%.1f s)\n", cart.hour(),
                    cart.minute(), cart.second(), cart.tries(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 PARADE CHIME  FAIL  %s  %d:%02d:%02d  try %d  left %d  phase %s  (%.1f s)\n", cart.reason(),
                cart.hour(), cart.minute(), cart.second(), cart.tries(), cart.lives(), cart.phase(), frames / 60.0);
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
            std::printf("S3 PARADE CHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3paradechime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<paradechime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
