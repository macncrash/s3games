// S3 INKWELL CHIME
//   s3inkwellchime                 dip until the hour chimes, then leave
//   s3inkwellchime --sim           autopilot, exits 0 only when the hour chimes
//   s3inkwellchime --sim --shots D also writes PNGs into D
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
    inkwellchime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (!cart.rules()) {
        std::printf("S3 INKWELL CHIME  FAIL  RULES  (0.0 s)\n");
        std::fflush(stdout);
        return 1;
    }
    bool titled = false, dipping = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 2) {
            save(sys, "desk.png");
            titled = true;
        }
        if (!dipping && cart.dips() > 0 && cart.dips() < 4) {
            save(sys, "well.png");
            dipping = true;
        }
    }
    save(sys, "end.png");
    bool hour = cart.onTheHour() && cart.hour() == 12 && cart.minute() == 0 && cart.second() < inkwellchime::kGraceSec;
    if (cart.won() && hour && cart.dips() == 4 && cart.tryNo() >= 1 && cart.tryNo() <= 3 &&
        std::strcmp(cart.reason(), "CHIME") == 0) {
        std::printf("S3 INKWELL CHIME  WIN  the hour chimes  %d:%02d:%02d  dips %d  try %d  (%.1f s)\n", cart.hour(),
                    cart.minute(), cart.second(), cart.dips(), cart.tryNo(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 INKWELL CHIME  FAIL  %s  %d:%02d:%02d  dips %d  try %d  (%.1f s)\n",
                cart.reason()[0] ? cart.reason() : "DRY", cart.hour(), cart.minute(), cart.second(), cart.dips(),
                cart.tryNo(), frames / 60.0);
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
            std::printf("S3 INKWELL CHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3inkwellchime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<inkwellchime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
