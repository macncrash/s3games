// S3 HOOPMARK
//   s3hoopmark                 set the coin, shoot, lift, leave
//   s3hoopmark --sim           autopilot finishes the mark
//   s3hoopmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/hoopmark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    hoopmark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, shot = false;
    int frames = 0;
    const int limit = 60 * 25;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 10) {
            save(sys, "title.png");
            titled = true;
        }
        if (!shot && cart.flying()) {
            save(sys, "shot.png");
            shot = true;
        }
    }
    save(sys, "end.png");
    bool made = !std::strcmp(cart.result(), "swish") || !std::strcmp(cart.result(), "count") ||
                !std::strcmp(cart.result(), "bank");
    if (cart.won() && cart.finished() && cart.lifted() && cart.opened() && cart.left() && cart.onMark() &&
        cart.shots() > 0 && made) {
        std::printf("S3 HOOPMARK  FINISHED MARK  %s  on the mark  coin lifted  left  shots %d  (%.1f s)\n",
                    cart.result(), cart.shots(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 HOOPMARK  OPEN  no finished mark  %s  shots %d  (%.1f s)\n", cart.result(), cart.shots(),
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
            std::printf("S3 HOOPMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3hoopmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<hoopmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
