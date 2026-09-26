#!/usr/bin/env python3
"""Write each cartridge's README and a small browser page.

The play link points at the one-file wasm build. This script does not
compile that file and does not record a local path.
"""
import html
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PLAY = "https://macncrash.github.io/s3games/play"

# Why the session is shaped this way, not a restatement of the rules.
DESIGN = {
    "s3baron": "The sight stays on the horizon and the guns are on the wings, so you lead the kite instead of the sprite under your nose. A plane that gets past comes around as the same debt, and the red one is only the last appointment.",
    "s3trap": "Pitch holds the donut and power holds the ball, which is how a real carrier pass is flown. There is no flare. The 3-wire is the aim, and the later sites reuse that same pass on a shorter, meaner deck.",
    "s3pins": "A mark is ten frames, not a high score you can grind. The pocket and the arrows are the whole decision, so a spare is a recovery and a strike is the only way to stay clean.",
    "s3tank": "One street, two machines. The block is won by whoever is still moving, so armor is a clock and not a health bar you hide behind.",
    "s3keel": "The triangle brings you back to the dock you left. A buoy you miss is not a penalty lap you can ignore. You have to come around and take it.",
    "s3putt": "Only the cup counts. Sitting on the green is not a score, so every stroke is aimed at the hole rather than at a safe lay-up.",
    "s3fort": "The gate is the score. You are not asked to clear the field. You are asked to still be holding the gate when the clock dies.",
    "s3ski": "Three missed gates end the run, so the line is a budget. Speed is useless if you spend the budget early.",
    "s3dart": "501 and double-out. The board is the whole game, and you cannot finish on a single. The last dart has to be the double you chose.",
    "s3clay": "Twenty-five birds, and the last five are the match. A hot start does not carry you. The pair at the end is what the score is for.",
    "s3rail": "The stop box is the station. Early and late both spoil the run, so the skill is arriving in the window, not beating the timetable by the most.",
    "s3curl": "Four ends, closest to the button. Weight and line are the only tools, and a stone that is close and guarded beats a stone that is closer and naked.",
    "s3raid": "You ride in and the yard has to be empty. Leaving one standing is not a partial win. The road home is the proof.",
    "s3kart": "Eight laps of one oval. The pack is the opponent, so a clean lap in last place is still a loss.",
    "s3eight": "Call the pocket before the 8. The rack is cleared only when that called ball goes where you said.",
    "s3mine": "The props are the targets and the walls are the fail. A cart that survives by not shooting has not finished the day.",
    "s3tug": "The ship has to sit in the slip. Touching a piling is a fail, so the last meters are slower than the approach.",
    "s3fair": "Three booths, then you leave. There is no prize case to fill. The score you walk out with is the game.",
    "s3blitz": "Ammunition is the clock. You do not get a separate timer. When the magazine is gone, the trench is over.",
    "s3metro": "Overshoot is a fail, not a door you can reopen. The box is small on purpose so the stop is the whole skill.",
    "s3hoop": "First to 21, and the rim is the judge. A ball that looks in and kicks out is out.",
    "s3yard": "Last machine running takes the purse. You are not scored on style. You are scored on being the one still moving.",
    "s3heli": "Three pads, and hovering the clock out is a loss. Each landing has to be a landing, then you go to the next pad.",
    "s3fish": "Daylight is the clock and five keepers is the bag. A sixth fish does not matter. Dark does.",
    "s3siege": "You are the wall. The rams are the problem, and the gate breaking ends it even if you still have people.",
    "s3sled": "The other sled is the clock. Beating a number on a board is not the race. Beating the sled next to you is.",
    "s3golf": "In the hole, not on the green. Three holes, and a ball next to the cup is the same as a ball in the trees.",
    "s3duel": "Three paces, then the draw. Firing early is a loss even if the shot would have hit.",
    "s3bus": "The doors open only in the box. A stop where you are close is a missed stop.",
    "s3wicket": "Six balls. You need the wicket and the rope, not one of them. A quiet over that does neither is a loss.",
    "s3convoy": "The truck arriving is the win. Your own hull is only the means. A hero who loses the truck has lost.",
    "s3ferry": "The tide clock is the deadline. Pretty handling after the tide has turned does not dock the boat.",
    "s3maze": "The exit is the only win. Mapping the hedge without leaving it does not count.",
    "s3rock": "A minute, and the hull is the score. You are not asked to destroy the rocks. You are asked to still be there.",
    "s3plow": "The pass has to be clear before the storm hits zero. A pretty stripe that is unfinished when the clock dies is a fail.",
    "s3chef": "Ten tickets, and a burned plate is not a served plate. Speed that ruins the food does not help the count.",
    "s3amber": "Runners are stopped and the ones who stop are spared. Hitting both, or neither, fails the intersection.",
    "s3crane": "Three crates on the mark. A drop is a failed shift, not a crate you pick up again for free.",
    "s3table": "First to seven, and the puck has to cross. A puck that dies on the line is not a point.",
    "s3outpost": "Flares show them, and dawn is the win. Clearing the field early and then leaving is not holding the post.",
    "s3drift": "One pass. The slide is the score and the wall is the end. A second run is not offered.",
    "s3arch": "Thirty-six arrows, gold counts double. You cannot skip to the last end. The card is the match.",
    "s3harbor": "One channel, the forts shoot back, and the magazine is finite. Saving rounds and not clearing the forts is a loss.",
    "s3orbit": "Soft dock or no dock. A hard contact on the arm fails even if you touched it.",
    "s3skate": "Land the line. A fall zeros the run, so a spectacular bail is worth nothing.",
    "s3keep": "Three minutes on the door. The score is that the door held, not how many you dropped.",
    "s3bike": "One kilometer and the wheels must not touch. Rubbing is a disqualification, not a racing incident you can shrug off.",
    "s3bocce": "First to seven, closest to the little ball. A cluster that is near but second does not take the end.",
    "s3flak": "The deck, the sky, a finite magazine. Splashing planes with no shells left, or keeping shells while the deck is hit, both fail.",
    "s3sub": "The channel is dark and the bottom is the fail. Depth is a limit, not a resource you spend.",
    "s3market": "Right change, and the line has to move. A correct till with a stalled queue is not a finished stall.",
    "s3ridge": "They come over the crest. Holding the path is the job. Chasing them off the path loses the ridge.",
    "s3glider": "Ridge lift gets you there. The field is the win and the trees are the fail. A long flight that lands in the trees is a crash.",
    "s3shuffle": "First to fifteen, and the disk has to stop in the score. A disk that slides through does not count.",
    "s3lot": "Three targets, then the gate, before the clock. Clearing the lot and missing the gate is not out.",
    "s3mower": "The stripes are the score. The field has to be cut before the rain, and a missed stripe is still uncut.",
    "s3keys": "One song. Five misses and the tune dies. You do not get a second chorus.",
    "s3span": "The column has to cross. Holding the span after they are across is the win. Dying with them still on it is not.",
    "s3luge": "One ice chute. Touching the wall ends the run. The clock is useless if you are in the wall.",
    "s3juggle": "Three in the air for a minute. A drop ends it. Extra flourish does not buy back a drop.",
    "s3depot": "The yard has to be clear. A load left on the dock is an unfinished shift.",
    "s3barge": "Enter, rise, leave. A scrape on the gate fails the lock even if you made the height.",
    "s3safe": "Three dials, and the room has the numbers. Guessing without reading the room is the slow way to lose.",
    "s3parade": "Reach the square. Getting hit is the fail. A high score that ends in the street is not a finish.",
    "s3therm": "One envelope. The mark is the landing and the trees are not. Time in the air is not the score.",
    "s3mosaic": "The picture has to complete. A pretty arrangement that is one tile off is not done.",
    "s3pouch": "The pouch has to cross three streets. Dropping it ends the run, so a fast crossing that loses the bag is a loss.",
    "s3rickshaw": "One fare, three turns, and the cab has to stay up. A tip ends the ride even if you made the turns.",
    "s3lantern": "Light the lamps in order. A miss hands one back to the dark, so skipping ahead undoes the chain.",
    "s3gauntlet": "The corridor, then the door. A high score that dies in the hall has not opened the door.",
    "s3row": "Five hundred meters, and you have to still be in the lane. A fast time out of the lane does not count.",
    "s3oven": "Six loaves, and a burned loaf fails the morning. Speed that ruins one loaf is not a finished bake.",
    "s3alley": "The far door is the win. Stumbling or falling behind in the street ends it short of that door.",
    "s3cable": "The tram has to stop level with the mark at each stop. Close is not level.",
    "s3memory": "Match the table before the clock. A pair left over when time dies is not a cleared table.",
    "s3bunker": "One room, one door. The room holding is the win, not the number you dropped outside it.",
    "s3combine": "One pass, and the header has to stay full. A fast pass that spills the header is a failed field.",
    "s3board": "Connect the calls before they drop. A line that rings and then falls does not count.",
    "s3torpedo": "Two shots, and the target has to sink. A hit that does not sink it spends a shot. Both spent with the ship still up is a miss.",
    "s3dune": "One water stop, and the engine must not boil. Reaching camp with a cooked engine is not a pass.",
    "s3striker": "Three swings to ring the bell. A swing that looks close and does not ring is a miss, not a point.",
    "s3eaves": "The far ladder is the win. A fall before it, however far you got, ends the crossing.",
    "s3logs": "The drive has to reach the boom. Sticks left in the river are an unfinished drive.",
    "s3choir": "Three entries have to land together. One voice arriving late stops the piece.",
    "s3militia": "Three waves, and the well has to stand. Clearing the field while the well falls is a loss.",
    "s3dogsled": "The checkpoint is the win, and the team turns with you. A spill ends the run even if the lantern is lit.",
    "s3shelve": "A book in the wrong row comes back, and the third return loses the cart. The win is an empty cart, not a pile of books you managed to keep.",
    "s3battery": "The column is the target and the gate is the fail. A gun still firing after a truck has passed has already lost.",
    "s3mail": "The paper has to land in the box, and the turn has to be made. A fast street that misses either one is not a finished route.",
    "s3solitaire": "One deal, face up, aces through sevens. Build down red on black and send them home in suit. A second shuffle is not this game.",
    "s3breach": "The banner has to come back through the door you broke. Reaching it and dying in the hall is not a rescue.",
    "s3lock": "One lock, and the gates are the fail. Making the height while scraping a leaf is not a clear pass.",
    "s3beds": "The sun on the wall is the deadline. Six beds watered after that are the same as beds left dry.",
    "s3rearguard": "The column getting home is the win. The fight is on the road behind it, and a score that loses the column is a loss.",
    "s3funicular": "The red floor has to stop level with the platform. Close is not level, and each of ORCHARD, PASS, and CREST has to be made.",
    "s3clock": "Three hands, and the rope only counts on a true hour. Hauling early does not chime.",
    "s3standard": "Reaching the flag is not the job. It has to come off the road and back through your own lines. A drop plants it in the road again.",
    "s3grass": "Three left-hand circuits of the grass strip. The third landing has to be a full stop. A touch-and-go on the last circuit is not a pass.",
    "s3drawer": "The drawer has to match the tape, then the shop closes. A till that is close is still open.",
    "s3gatereli": "Hold the gate until the relief bell, then answer it. Surviving the watch and missing the bell is not relief.",
    "s3skiffbuoy": "Round the buoys and take the same dock you left. A fast lap that finishes at the wrong end fails the leg.",
    "s3pinsmark": "A strike or a spare opens a mark, and that finished mark ends it. Bowling out the rest of the card is not the job.",
    "s3gatecolu": "The column has to stop on the road. A truck that gets through is a loss, whatever else you hit.",
    "s3skiffbox": "Stop inside the box before the other crew. Close to the box is still outside.",
    "s3pinsgold": "Gold counts double, and the frame has to finish on a double. Cream pins do not buy back a missed double.",
    "s3gatebann": "The banner has to come back through the gate. Reaching it and staying on the far side is not done.",
    "s3skifflock": "One lock. A scrape on a gate fails the pass even if the skiff made it through.",
    "s3pinsseven": "First bowler to seven. A leave that looks close and is still under seven is not the game.",
    "s3gatepurs": "Be the last machine still running. Stopping the others and then dying yourself does not take the purse.",
    "s3skiffgrass": "Land on the grass and come to a full stop. A touch that keeps rolling fails the leg.",
    "s3pinsbell": "The bell has to ring before the third try dies. A clean leave after the third try is already over.",
    "s3gatewell": "The well has to stand through three waves. A stone that gets through is a loss even if you are still at the gate.",
    "s3skiffkilo": "Finish the kilometer with the wheels untouched and time left on the other crew's clock. A scrape spends the run even if you make the distance.",
    "s3pinschime": "The hour has to chime. Clearing the pins while the clock is still short of the hour is not this cartridge.",
    "s3gatepace": "Fire on the third pace. An earlier shot that still holds the gate is not the job.",
    "s3skiffpass": "Clear the pass before the storm clock dies. Making the pass with the clock already gone fails it.",
    "s3pinstape": "The drawer has to match the tape, and that closes the short rack. A score that does not match the tape is still open.",
    "s3gatemaga": "The magazine has to outlast the raid. Emptying it before the raid ends fails the watch.",
    "s3skiffslip": "Berth in the slip before the tide turns. A late berth fails the leg even if you find the slip.",
    "s3puttmark": "The coin on the ball opens the mark, and lifting it after the hole finishes the mark. The rest of a card is not the job.",
    "s3gatedawn": "The flares have to stay lit until dawn. A dark flare before dawn is a loss.",
    "s3skiffboom": "The drive has to land on the boom ahead of the other crew. A late delivery fails the leg even if it arrives.",
    "s3puttgold": "Gold counts double. A cream cup does not buy the double.",
    "s3gatedoor": "The door has to hold for three minutes. Opening it early ends the watch.",
    "s3skiffplat": "Stop level with the platform. Close is not level.",
    "s3puttseven": "First to seven holes, then leave. A six that looks close is still short.",
    "s3gatepouc": "The pouch has to cross. Reaching the far side without it ends the watch.",
    "s3skifflane": "Stay between the buoys for the whole leg. Leaving the lane fails it even if you reach the gate.",
    "s3puttbell": "The bell has to ring before the third try dies. A later hole is already over.",
    "s3gateladd": "The far ladder is the job. Stopping short of it is a loss.",
    "s3skiffmark": "Set down on the mark ahead of the other crew. Close to the mark, or late, fails the leg.",
    "s3puttchime": "The hour has to chime. A holed putt before the hour is not this cartridge.",
    "s3gatecler": "Clear the ground before the clock dies. A clean yard after the clock is already over.",
    "s3skiffturn": "Three bends without tipping. A heel that puts the skiff over fails the creek even if you made the turns.",
    "s3putttape": "The drawer has to match the tape, then you leave. A till that is close is still open.",
    "s3ridgereli": "Hold the ridge until the relief bell, then haul the rope. Surviving the watch and missing the bell is not relief.",
    "s3gliderbuoy": "Round the buoys and set down on the same dock ahead of the other crew. A fast circuit that finishes at the wrong dock fails the leg.",
    "s3dartmark": "A treble 20 closes the 20 and finishes the mark. The rest of a visit is not the job.",
    "s3ridgecolu": "The column has to stop on the road. A truck that gets through is a loss.",
    "s3gliderbox": "Stop inside the box. Close to the box is still outside.",
    "s3dartgold": "Five-oh-one, and only a gold bed counts as a double. A cream double does not check out.",
    "s3ridgebann": "The banner has to come back onto the ridge. Reaching it and staying off the ridge ends the watch.",
    "s3gliderlock": "Pass the lock without scraping a gate. A scrape fails the leg even if you made it through.",
    "s3dartseven": "One dart apiece, first to seven. A leave that looks close and is still under seven is not the game.",
    "s3ridgepurs": "Be the last machine still running. Stopping the others and then dying yourself does not take the ridge.",
    "s3glidergrass": "Land on the grass and come to a full stop ahead of the other crew. A touch that keeps rolling fails the leg.",
    "s3dartbell": "The bell has to ring before the third try dies. A later dart is already over.",
    "s3ridgewell": "The well has to stand through three waves. A stone that gets through is a loss.",
    "s3gliderkilo": "Finish the kilometer with the wheels untouched. A scrape spends the run even if you make the distance.",
}


def catalog_rows():
    rows = []
    for line in (ROOT / "CATALOG.md").read_text().splitlines():
        m = re.match(r"\| (\d+) \| (s3[a-z0-9]+) \| ([^|]+) \| ([a-z]+) \| ([^|]+) \|", line)
        if not m:
            continue
        num, slug, lead, state, blurb = m.groups()
        if slug == "s3garally":
            continue
        if slug in ("s3baron", "s3trap") or state == "complete":
            rows.append((int(num), slug, lead.strip(), blurb.strip().replace("`", "")))
    rows.sort()
    return rows


def title_of(slug):
    return slug[2:].upper() if slug.startswith("s3") else slug.upper()


def readme(slug, blurb, design):
    play = f"{PLAY}/{slug}/"
    return (
        f"# S3 {title_of(slug)}\n\n"
        f"{blurb}\n\n"
        f"Play it in the browser: {play}\n\n"
        f"## Design\n\n"
        f"{design}\n\n"
        f"The browser build is one file. The link above is that file, not a capture of a desktop session.\n"
    )


def page(slug, blurb, design):
    play = f"../../play/{slug}/"
    body = html.escape(blurb)
    note = html.escape(design)
    name = html.escape(f"S3 {title_of(slug)}")
    return f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{name}</title>
<style>
  body {{ margin: 0; background: #101014; color: #f2f2f4; font: 16px/1.5 ui-monospace, Menlo, monospace; }}
  main {{ max-width: 640px; margin: 0 auto; padding: 40px 20px; }}
  a.play {{ display: inline-block; margin: 16px 0; padding: 10px 16px; background: #ffc21a; color: #101014; text-decoration: none; font-weight: 700; }}
  a.back {{ color: #ffc21a; }}
  p {{ color: #d5d5dc; }}
</style>
</head>
<body>
<main>
<p><a class="back" href="../../index.html">All games</a></p>
<h1>{name}</h1>
<p>{body}</p>
<p><a class="play" href="{play}">Play</a></p>
<h2>Design</h2>
<p>{note}</p>
</main>
</body>
</html>
"""


def main():
    for _num, slug, _lead, blurb in catalog_rows():
        design = DESIGN.get(slug)
        if not design:
            raise SystemExit(f"missing design note for {slug}")
        game = ROOT / slug
        if not any((game / "src").rglob("*.cpp")):
            continue
        readme_path = game / "README.md"
        text = readme(slug, blurb, design)
        if readme_path.is_file() and "Play it in the browser:" in readme_path.read_text():
            pass
        elif readme_path.is_file() and slug in ("s3baron", "s3trap"):
            old = readme_path.read_text()
            banner = f"Play it in the browser: {PLAY}/{slug}/\n\n## Design\n\n{design}\n\n"
            if "Play it in the browser:" not in old:
                readme_path.write_text(banner + old)
        else:
            readme_path.write_text(text)
        page_path = ROOT / "web" / "games" / slug / "index.html"
        page_path.parent.mkdir(parents=True, exist_ok=True)
        page_path.write_text(page(slug, blurb, design))
    print(f"wrote pages for {len(list(catalog_rows()))} games")


if __name__ == "__main__":
    main()
