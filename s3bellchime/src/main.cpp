// S3 BELLCHIME
//   s3bellchime                 play the short bell
//   s3bellchime --sim           the hour chimes, or the run fails
//   s3bellchime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/bellchime.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    bellchime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 20;
    bool titled = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 4) {
            save(sys, "title.png");
            titled = true;
        }
    }
    save(sys, "end.png");
    bool hour = cart.onTheHour() && cart.hour() == 12 && cart.minute() == 0 && cart.second() < bellchime::kGraceSec;
    if (cart.won() && hour && cart.pulls() >= 1 && cart.pulls() <= bellchime::kPulls &&
        std::strcmp(cart.reason(), "CHIME") == 0 && std::strcmp(cart.bell(), "SHORT") == 0) {
        std::printf("S3 BELLCHIME  WIN  the hour chimes  %d:%02d:%02d  SHORT  pull %d  (%.1f s)\n", cart.hour(),
                    cart.minute(), cart.second(), cart.pulls(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 BELLCHIME  FAIL  %s  %d:%02d:%02d  %s  pull %d  (%.1f s)\n",
                cart.reason()[0] ? cart.reason() : "OPEN", cart.hour(), cart.minute(), cart.second(),
                cart.bell()[0] ? cart.bell() : "-", cart.pulls(), frames / 60.0);
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
            std::printf("S3 BELLCHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3bellchime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<bellchime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
