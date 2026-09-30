// S3 SCORE CHIME
//   s3scorechime                 a short score; the hour has to chime
//   s3scorechime --sim           autopilot leaves while the hour is chiming
//   s3scorechime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/scorechime.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    scorechime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (frames == 8) save(sys, "title.png");
        if (frames == 70) save(sys, "score.png");
    }
    save(sys, "end.png");
    const bool hour =
        cart.onTheHour() && cart.hour() == 12 && cart.minute() == 0 && cart.second() < scorechime::kGraceSec;
    if (cart.won() && hour && cart.marks() == scorechime::kMarks && cart.misses() == 0 &&
        std::strcmp(cart.why(), "CHIME") == 0) {
        std::printf("S3 SCORECHIME  WIN  the hour chimes  %d:%02d:%02d  marks %d  (%.1f s)\n", cart.hour(),
                    cart.minute(), cart.second(), cart.marks(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    const char* why = cart.over() ? cart.why() : "timed out";
    std::printf("S3 SCORECHIME  FAIL  %s  %d:%02d:%02d  marks %d  misses %d  (%.1f s)\n", why, cart.hour(),
                cart.minute(), cart.second(), cart.marks(), cart.misses(), frames / 60.0);
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
            std::printf("S3 SCORE CHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3scorechime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<scorechime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
