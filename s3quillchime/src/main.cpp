// S3 QUILLCHIME
//   s3quillchime                 play the short quill
//   s3quillchime --sim           the hour chimes, or the run fails
//   s3quillchime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/quillchime.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    quillchime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 25;
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
    bool hour = cart.onTheHour() && cart.hour() == 12 && cart.minute() == 0 && cart.second() < quillchime::kGraceSec;
    if (cart.won() && hour && cart.ink() >= quillchime::kInkLo && cart.ink() <= quillchime::kInkHi &&
        cart.dips() < quillchime::kDips && std::strcmp(cart.reason(), "CHIME") == 0 &&
        std::strcmp(cart.face(), "SHORT") == 0) {
        std::printf("S3 QUILLCHIME  WIN  the hour chimes  %d:%02d:%02d  SHORT  ink %d  (%.1f s)\n", cart.hour(),
                    cart.minute(), cart.second(), cart.ink(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 QUILLCHIME  FAIL  %s  %d:%02d:%02d  %s  ink %d  (%.1f s)\n",
                cart.reason()[0] ? cart.reason() : "OPEN", cart.hour(), cart.minute(), cart.second(),
                cart.face()[0] ? cart.face() : "-", cart.ink(), frames / 60.0);
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
            std::printf("S3 QUILLCHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3quillchime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<quillchime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
