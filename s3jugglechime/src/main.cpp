// S3 JUGGLECHIME
//   s3jugglechime                 juggle until the hour has to chime, then leave
//   s3jugglechime --sim           autopilot, exits 0 only when the hour chimes
//   s3jugglechime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/jugglechime.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    jugglechime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (!cart.rules()) {
        std::printf("S3 JUGGLECHIME  FAIL  RULES  %d:%02d:%02d  (0.0 s)\n", cart.hour(), cart.minute(), cart.second());
        std::fflush(stdout);
        return 1;
    }
    bool titled = false;
    int frames = 0;
    const int limit = 60 * 25;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
    }
    save(sys, "end.png");
    bool hour = cart.onTheHour() && cart.hour() == 12 && cart.minute() == 0 && cart.second() < jugglechime::kGraceSec;
    if (cart.won() && hour && cart.live() && cart.catches() > 0 && cart.strikes() >= 12 &&
        std::strcmp(cart.reason(), "CHIME") == 0) {
        std::printf("S3 JUGGLECHIME  WIN  the hour chimes  %d:%02d:%02d  catches %d  (%.1f s)\n", cart.hour(),
                    cart.minute(), cart.second(), cart.catches(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 JUGGLECHIME  FAIL  %s  %d:%02d:%02d  catches %d  (%.1f s)\n",
                cart.reason()[0] ? cart.reason() : "OPEN", cart.hour(), cart.minute(), cart.second(), cart.catches(),
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
            std::printf("S3 JUGGLECHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3jugglechime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<jugglechime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
