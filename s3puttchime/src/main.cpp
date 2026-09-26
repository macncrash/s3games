// S3 PUTTCHIME
//   s3puttchime                 a short putt; the hour has to chime
//   s3puttchime --sim           autopilot holes the cup as the hour strikes
//   s3puttchime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/puttchime.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    puttchime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, rolling = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 16) {
            save(sys, "title.png");
            titled = true;
        }
        if (!rolling && cart.rolling()) {
            save(sys, "roll.png");
            rolling = true;
        }
    }
    save(sys, "end.png");
    bool hour = cart.hour() == 12 && cart.minute() == 0 && cart.second() < 24;
    if (cart.won() && cart.holed() && hour && cart.putts() >= 1 && cart.putts() <= 3 &&
        std::strcmp(cart.reason(), "CHIME") == 0) {
        std::printf("S3 PUTTCHIME  WIN  the hour chimes  %d:%02d:%02d  putt %d  (%.1f s)\n", cart.hour(), cart.minute(),
                    cart.second(), cart.putts(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 PUTTCHIME  FAIL  %s  %d:%02d:%02d  putt %d  (%.1f s)\n", cart.reason(), cart.hour(), cart.minute(),
                cart.second(), cart.putts(), frames / 60.0);
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
            std::printf("S3 PUTTCHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3puttchime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<puttchime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
