// S3 PAWNCHIME
//   s3pawnchime                 play pawn until the hour has to chime
//   s3pawnchime --sim           autopilot, exits 0 only when the hour chimes
//   s3pawnchime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/pawn.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    pawnchime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (frames == 8) save(sys, "title.png");
        if (frames == 90) save(sys, "file.png");
    }
    save(sys, "end.png");
    const bool hour =
        cart.onTheHour() && cart.hour() == 12 && cart.minute() == 0 && cart.second() < pawnchime::kGraceSec;
    if (cart.won() && hour && cart.steps() == pawnchime::kSteps && cart.misses() == 0 &&
        std::strcmp(cart.why(), "CHIME") == 0) {
        std::printf("S3 PAWNCHIME  WIN  the hour chimes  %d:%02d:%02d  steps %d  (%.1f s)\n", cart.hour(),
                    cart.minute(), cart.second(), cart.steps(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    const char* why = cart.over() ? cart.why() : "timed out";
    std::printf("S3 PAWNCHIME  FAIL  %s  %d:%02d:%02d  steps %d  misses %d  (%.1f s)\n", why, cart.hour(),
                cart.minute(), cart.second(), cart.steps(), cart.misses(), frames / 60.0);
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
            std::printf("S3 PAWNCHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3pawnchime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<pawnchime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
