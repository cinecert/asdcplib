/*
Copyright (c) 2005-2018, John Hurst
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
  /*! \file    kmrandgen.cpp
    \version $Id$
    \brief   psuedo-random number generation utility
  */

#include "AS_DCP.h"
#include <KM_fileio.h>
#include <KM_prng.h>
#include <ctype.h>

using namespace Kumu;

const ui32_t RandBlockSize = 16;
const char* PROGRAM_NAME = "kmrandgen";

// Increment the iterator, test for an additional non-option command line argument.
// Causes the caller to return if there are no remaining arguments or if the next
// argument begins with '-'.
#define TEST_EXTRA_ARG(i,c)    if ( ++i >= argc || argv[(i)][0] == '-' ) \
                                 { \
                                   fprintf(stderr, "Argument not found for option -%c.\n", (c)); \
                                   return; \
                                 }

static const char* _letterwords_list[] = {
  "Alfa", "Bravo", "Charlie", "Delta", "Echo", "Foxtrot", "Golf",
  "Hotel", "India", "Juliett", "Kilo",
  "Lima", "Mike", "November", "Oscar", "Papa",
  "Quebec", "Romeo", "Sierra",
  "Tango", "Uniform", "Victor",
  "Whiskey", "X-ray",
  "Yankee", "Zulu"
};


static const char* _word_list[] = {
  "ABED", "ABET", "ABEY", "ABLE", "ABUT",  "ACE", "ACHE", "ACID", 
  "ACME", "ACRE",  "ACT", "ACTS",  "ADD", "ADDS",  "ADO", "ADRY", 
  "AEON", "AERO", "AFAR", "AFRO", "AGAR",  "AGE", "AGED", "AGIO", 
  "AGO" , "AHEM", "AHOY",  "AID", "AIDE",  "AIL",  "AIM",  "AIR", 
  "AIRY", "AJAR", "AKIN", "ALAS", "ALCO",  "ALE",  "ALF", "ALFA", 
  "ALL" , "ALLY", "ALMA", "ALMS", "ALOE", "ALOW", "ALSO",  "ALT", 
  "ALTO", "ALUM",   "AM", "AMES", "AMID",   "AN",  "AND", "ANEW", 
  "ANT" , "ANTE",  "ANY",  "APE", "APEX",  "APT", "AQUA",  "ARC", 
  "ARCH",  "ARE", "AREA", "ARGO", "ARID",  "ARK",  "ARM", "ARMS", 
  "ART" , "ARTS", "ARTY",   "AS",  "ASH",  "ASK", "ASKS",  "ASP", 
  "AT"  ,  "ATE", "ATOM", "ATOP", "AUNT", "AURA", "AUTO", "AVER", 
  "AVID", "AVOW", "AWAY",  "AWE",  "AWL",  "AXE", "AXED", "AXES", 
  "AXIS", "AXLE", "AXON",  "AYE", "BABE", "BABU", "BABY", "BACK", 
  "BAD" , "BADE",  "BAG", "BAIL", "BAIT", "BAKE", "BALD", "BALE", 
  "BALK", "BALL", "BALM",  "BAM",  "BAN", "BAND", "BANE", "BANG", 
  "BANK",  "BAR", "BARB", "BARD", "BARE", "BARK", "BARM", "BARN", 
  "BARU", "BASE", "BASH", "BASK", "BASS",  "BAT", "BATH", "BATS", 
  "BAWL",  "BAY",   "BE", "BEAD", "BEAK", "BEAM", "BEAN", "BEAR", 
  "BEAT", "BEAU", "BECK",  "BED",  "BEE", "BEEF", "BEEK", "BEEN", 
  "BEER", "BEES", "BEET",  "BEG", "BELL", "BELT", "BEND", "BENT", 
  "BERG", "BERM", "BEST",  "BET", "BETA", "BIAS",  "BIB",  "BID", 
  "BIDE", "BIER",  "BIG", "BIKE", "BILE", "BILK", "BILL",  "BIN", 
  "BIND", "BING", "BIOS", "BIRD",  "BIT", "BITE", "BITS",  "BIZ", 
  "BLAB", "BLED", "BLOB", "BLOC", "BLOK", "BLOT", "BLUE", "BLUR", 
  "BOA" , "BOAR", "BOAT",  "BOB", "BOCE", "BOCK", "BODE", "BODY", 
  "BOG" , "BOIL", "BOLD", "BOLO", "BOLT", "BOND", "BONE", "BONY", 
  "BOO" , "BOOK", "BOOL", "BOOM", "BOON", "BOOR", "BOOT", "BORE", 
  "BORG", "BORN", "BOSS", "BOTH", "BOUT",  "BOW", "BOWL", "BOXY", 
  "BOY" , "BRAG", "BRAN", "BRAT", "BRAY", "BRED", "BREW", "BRIE", 
  "BRIG", "BRIM", "BRIT", "BROW", "BUCK",  "BUD", "BUFF",  "BUG", 
  "BULB", "BULK", "BULL", "BUMP",  "BUN", "BUNK", "BUNT", "BUOY", 
  "BUR" , "BURG", "BURL", "BURN", "BURP", "BURR", "BURY",  "BUS", 
  "BUSH", "BUSK", "BUSS", "BUSY",  "BUT",  "BUY",   "BY",  "BYE", 
  "BYTE",  "CAB",  "CAD", "CAFE", "CAGE", "CAIN", "CAKE",  "CAL", 
  "CALF", "CALL", "CALM",  "CAM", "CAMP",  "CAN", "CANE", "CANT", 
  "CAP" , "CAPE",  "CAR", "CARD", "CARE", "CARP", "CART", "CASE", 
  "CASH", "CASK", "CAST",  "CAT", "CAVE", "CEDE", "CEIL", "CELL", 
  "CENT", "CHAP", "CHAT", "CHEF", "CHEW", "CHIC", "CHIN", "CHIP", 
  "CHIT", "CHOP", "CHOW", "CHUG", "CHUM", "CINE", "CITE", "CITY", 
  "CLAD", "CLAM", "CLAN", "CLAP", "CLAW", "CLAY", "CLEF", "CLIP", 
  "CLOD", "CLOG", "CLOT", "CLOY", "CLUB", "CLUE", "COAL", "COAT", 
  "COAX",  "COB",  "COD", "CODA", "CODE",  "COG", "COIL", "COIN", 
  "COKE", "COLA", "COLD", "COLE", "COLT", "COMB", "COOK", "COOL", 
  "COP" , "COPE", "COPY", "CORD", "CORE", "CORK", "CORN", "CORP", 
  "COST", "COSY",  "COT", "COUP", "COVE",  "COW", "COWL",  "COY", 
  "COZY", "CRAB", "CRAG", "CRAM", "CRAW", "CRAY", "CREW", "CRIB", 
  "CROC", "CROP", "CROW", "CRUX",  "CRY",  "CUB", "CUBE",  "CUE", 
  "CUFF",  "CUP",  "CUR", "CURB", "CURD", "CURE", "CURL", "CURT", 
  "CUT" , "CUTE", "CYAN", "CYST", "CZAR",  "DAB",  "DAD", "DADA", 
  "DADO", "DAIS", "DALE", "DALI",  "DAM", "DAME", "DAMP", "DARE", 
  "DARK", "DARN", "DART", "DASH", "DATA", "DATE", "DAUB", "DAWN", 
  "DAY" , "DAYS", "DAZE", "DAZY", "DEAL", "DEAR", "DEBT", "DECK", 
  "DEED", "DEEM", "DEER", "DEFT", "DEFY", "DELI", "DELL", "DEMO", 
  "DEN" , "DENT", "DENY", "DESK",  "DEW", "DIAL", "DIBS", "DICE", 
  "DID" ,  "DIG", "DIGS", "DILL",  "DIM", "DIME",  "DIN", "DINE", 
  "DIP" , "DIRE", "DIRT", "DISC", "DISH", "DISK", "DIVE",   "DO", 
  "DOCK",  "DOE", "DOES",  "DOG", "DOGS", "DOLE", "DOLL", "DOME", 
  "DON" , "DONE", "DOOM", "DOOR", "DORM", "DOSE",  "DOT", "DOTE", 
  "DOUR", "DOVE", "DOWN", "DOZE", "DRAB", "DRAG", "DRAM", "DRAT", 
  "DRAW", "DREW", "DRIB", "DRIP", "DROP", "DRUB", "DRUM",  "DRY", 
  "DUAL",  "DUB", "DUBS", "DUCK", "DUCT",  "DUD",  "DUE", "DUET", 
  "DUG" , "DUKE", "DULL", "DULY", "DUMP", "DUNE", "DUNK", "DUPE", 
  "DUSK", "DUST", "DUTY", "DYAD", "EACH",  "EAR", "EARL", "EARN", 
  "EASE", "EAST", "EASY",  "EAT", "EATS",  "EBB", "ECHO", "EDDY", 
  "EDGE", "EDGY", "EDIT",  "EEL",  "EGG",  "EGO",  "ELF",  "ELK", 
  "ELM" , "ELSE", "EMIT",  "END", "ENDS", "ENSE", "ENVY", "EPIC", 
  "ERA" ,  "ERG",  "EVE", "EVEN", "EVER",  "EWE", "EXAM", "EXIT", 
  "EYE" , "EYED", "FACE", "FACT",  "FAD", "FADE", "FADY", "FAIL", 
  "FAIR", "FAKE", "FALL", "FAME",  "FAN", "FANG",  "FAR", "FARE", 
  "FARM", "FAST", "FATE", "FAWN", "FAZE", "FEAR", "FEAT",  "FED", 
  "FEE" , "FEED", "FEEL", "FEET", "FELL", "FELT", "FEND", "FERN", 
  "FEST", "FEUD",  "FEW",  "FEZ", "FIAT",  "FIB", "FIFE",  "FIG", 
  "FIGS", "FILE", "FILL", "FILM",  "FIN", "FIND", "FINE", "FINK", 
  "FIR" , "FIRE", "FIRM", "FISH",  "FIT", "FITS", "FIVE",  "FIX", 
  "FLAG", "FLAK", "FLAP", "FLAT", "FLAW", "FLAX", "FLED", "FLEE", 
  "FLEW", "FLEX", "FLIP", "FLIT", "FLOE", "FLOG", "FLOP", "FLOW", 
  "FLU" , "FLUB", "FLUE", "FLUX",  "FLY", "FOAL", "FOAM",  "FOE", 
  "FOG" , "FOIL", "FOLD", "FOLK", "FOND", "FONT", "FOOD", "FOOL", 
  "FOOT", "FORD", "FORE", "FORK", "FORM", "FORT", "FOUL", "FOUR", 
  "FOW" , "FOWL",  "FOX", "FRAY", "FRED", "FREE", "FRET", "FROG", 
  "FROM",  "FRY", "FUEL", "FUGU", "FULL", "FUME",  "FUN", "FUND", 
  "FUNK",  "FUR", "FURY", "FUSE", "FUSS", "FUZZ",  "GAB",  "GAG", 
  "GAGE", "GAIN", "GAIT",  "GAL", "GALA", "GALE", "GALL", "GAME", 
  "GAMY", "GANG",  "GAP", "GARB",  "GAS", "GASP", "GATE", "GAVE", 
  "GAWK", "GAZE", "GEAR", "GEEK",  "GEL",  "GEM", "GENE", "GENT", 
  "GERM",  "GET", "GETS", "GIFT",  "GIG", "GILD", "GILL", "GILT", 
  "GIN" , "GIRD", "GIRL", "GIST",  "GIT", "GIVE", "GLAD", "GLAM", 
  "GLEE", "GLEN", "GLIB", "GLOB", "GLOM", "GLOP", "GLOW", "GLUE", 
  "GLUG", "GLUM", "GLUT", "GNAT", "GNAW",  "GNU",   "GO", "GOAD", 
  "GOAL", "GOAT",  "GOB", "GOBO", "GOES", "GOLD", "GOLF", "GONE", 
  "GONG",  "GOO", "GOOD", "GOOF", "GOON", "GOSH",  "GOT", "GOWN", 
  "GRAB", "GRAD", "GRAM", "GRAY", "GREW", "GREY", "GRID", "GRIM", 
  "GRIN", "GRIP", "GRIT", "GROG", "GROW", "GRUB", "GRUE", "GULF", 
  "GULL", "GULP",  "GUM", "GUNK", "GURU", "GUSH", "GUST",  "GUT", 
  "GUY" ,  "GYM", "HACK",  "HAD",  "HAH", "HAIL", "HAIR", "HALE", 
  "HALF", "HALL", "HALO", "HALT",  "HAM", "HAND", "HANG", "HANK", 
  "HARD", "HARE", "HARK", "HARM", "HARP",  "HAS", "HASH", "HASP", 
  "HAT" , "HATH", "HAUL", "HAVE", "HAWK",  "HAY", "HAZE", "HAZY", 
  "HE"  , "HEAL", "HEAP", "HEAR", "HEAT", "HECK", "HEED", "HEEL", 
  "HEFT", "HEIR", "HELD", "HELM", "HELP",  "HEM", "HEMP",  "HEN", 
  "HER" , "HERB", "HERD", "HERE", "HERO", "HERS",  "HEW", "HEWN", 
  "HEX" ,  "HEY",   "HI",  "HID", "HIDE", "HIGH", "HIKE", "HILL", 
  "HILT",  "HIM", "HIND", "HINT",  "HIP", "HIRE",  "HIS", "HISS", 
  "HIT" , "HIVE", "HOAX", "HOCK",  "HOE",  "HOG", "HOLD", "HOLE", 
  "HOME", "HONE", "HONK", "HOOD", "HOOF", "HOOK", "HOOP", "HOOT", 
  "HOP" , "HOPE", "HORN", "HOSE", "HOST",  "HOT", "HOUR",  "HOW", 
  "HOWL",  "HUB",  "HUE", "HUED", "HUFF",  "HUG", "HUGE",  "HUH", 
  "HULK", "HULL",  "HUM", "HUNK", "HUNT", "HURL", "HURT", "HUSH", 
  "HUSK",  "HUT", "HYMN", "HYPO",  "ICE", "ICON",  "ICY",   "ID", 
  "IDEA", "IDES", "IDLE", "IDLY", "IDOL",   "IF", "IFFY",  "ILK", 
  "ILL" ,  "IMP",   "IN", "INCH", "INDY",  "INK",  "INN", "INTO", 
  "ION" , "IONS", "IOTA", "IRIS", "IRON",   "IS", "ISLE",   "IT", 
  "ITEM",  "IVY", "JADE",  "JAG",  "JAM",  "JAR", "JAVA",  "JAW", 
  "JAZZ", "JEDI", "JEEP", "JEST",  "JET",  "JIB",  "JIG", "JILT", 
  "JIVE",  "JOB", "JOBS",  "JOG", "JOIN", "JOKE", "JOLT",  "JOT", 
  "JUDO",  "JUG", "JULY", "JUMP", "JUNE", "JUNK", "JURY", "JUST", 
  "JUT" , "KAHN", "KALE", "KANE", "KEEL", "KEEN", "KEEP",  "KEG", 
  "KELP", "KENO", "KEPT", "KERF", "KERN",  "KEY", "KEYS", "KHAN", 
  "KICK",  "KID", "KILN", "KILO", "KILT",  "KIN", "KIND", "KING", 
  "KINO",  "KIT", "KITE", "KNEE", "KNEW", "KNIT", "KNOT", "KNOW", 
  "KOI" ,  "LAB", "LACE", "LACK",  "LAD", "LADY",  "LAG", "LAIR", 
  "LAKE",  "LAM", "LAMB", "LAME", "LAMP", "LAND", "LANE",  "LAP", 
  "LARD", "LARK", "LASH", "LASS", "LAST", "LATE", "LAUD", "LAVA", 
  "LAW" , "LAWN", "LAWS",  "LAX",  "LAY", "LAZY", "LEAD", "LEAF", 
  "LEAK", "LEAN", "LEAP",  "LED", "LEDE", "LEED", "LEEK", "LEFT", 
  "LEG" , "LEND", "LENS", "LENT", "LESS", "LEST",  "LET", "LETS", 
  "LIAR",  "LID", "LIEN", "LIEU", "LIFE", "LIFO", "LIFT", "LIKE", 
  "LILT", "LILY", "LIMA", "LIME", "LINE", "LINK", "LINT", "LION", 
  "LIP" , "LIST",  "LIT", "LIVE", "LOAD", "LOAF", "LOAN",  "LOB", 
  "LOBE", "LOCK", "LODE", "LOFT",  "LOG", "LOGE", "LOIN", "LONE", 
  "LONG", "LOOK", "LOON", "LOOP", "LOOT", "LORE", "LOSE", "LOSS", 
  "LOST",  "LOT", "LOTS", "LOUD", "LOVE",  "LOW",  "LOX", "LUCK", 
  "LUG" , "LULL", "LUMP", "LUSH", "LUTE",  "LUX",  "LYE", "LYNX", 
  "MAD" , "MADE", "MAID", "MAIL", "MAIN", "MAKE", "MALE", "MALL", 
  "MALT",  "MAN", "MANY",  "MAP", "MARE", "MARK", "MART", "MASH", 
  "MASK", "MASS", "MAST",  "MAT", "MATE", "MATH", "MAUL",  "MAW", 
  "MAY" , "MAZE",   "ME", "MEAD", "MEAL", "MEAN", "MEAT", "MEEK", 
  "MEET", "MELD", "MELT", "MEME", "MEMO",  "MEN", "MEND", "MENU", 
  "MERE", "MESH", "MESS",  "MET", "MICA", "MICE",  "MID", "MIKE", 
  "MILD", "MILE", "MILK", "MILL", "MIME", "MIND", "MINE", "MINK", 
  "MINT", "MIRE", "MISS", "MIST", "MITE", "MITT",  "MIX", "MOAT", 
  "MOB" , "MOCK",  "MOD", "MODE", "MOLD", "MOLE", "MOLT", "MONK", 
  "MONO", "MOOD", "MOON", "MOOT",  "MOP", "MOPE", "MORE", "MORN", 
  "MOSS", "MOST", "MOTE", "MOTH", "MOVE",  "MOW", "MOWN", "MUCH", 
  "MUCK",  "MUD",  "MUG", "MULE", "MULL", "MULT",  "MUM", "MUMP", 
  "MURK", "MUSE", "MUSH", "MUSK", "MUSS", "MUST", "MUTE", "MUTT", 
  "MY"  , "MYTH",  "NAB",  "NAG", "NAIF", "NAIL", "NAME",  "NAP", 
  "NAPE", "NARY", "NEAR", "NEAT",  "NEE", "NEED", "NEON", "NEST", 
  "NET" ,  "NEW", "NEWS", "NEWT", "NEXT",  "NIB", "NIBS", "NICE", 
  "NICK",  "NIL", "NINE",  "NIX",   "NO",  "NOD", "NODE", "NONE", 
  "NOOK", "NOON", "NOPE",  "NOR", "NORI", "NORM", "NOSE", "NOSY", 
  "NOT" , "NOTE", "NOUN", "NOVA",  "NOW", "NULL", "NUMB",  "NUN", 
  "OAK" ,  "OAR",  "OAT", "OATH", "OBOE",  "ODD", "ODDS",  "ODE", 
  "OF"  ,  "OFF",  "OFT", "OGRE",   "OH", "OHIO",  "OHM",  "OIL", 
  "OILY",   "OK", "OKAY", "OKRA",  "OLD", "OMEN", "OMIT",   "ON", 
  "ONCE",  "ONE", "ONES", "ONLY", "ONTO", "ONUS", "ONYX", "OPAL", 
  "OPEN",  "OPT", "OPUS",   "OR",  "ORB",  "ORC",  "ORE", "OUCH", 
  "OUR" , "OURS", "OUST",  "OUT", "OUTS", "OVAL", "OVEN", "OVER", 
  "OWE" ,  "OWL",  "OWN", "OWNS",   "OX", "PACK", "PACT",  "PAD", 
  "PAGE", "PAIL", "PAIN", "PAIR",  "PAL", "PALE", "PALL", "PALM", 
  "PAN" , "PANE", "PAPA",  "PAR", "PARK", "PART", "PASS", "PAST", 
  "PAT" , "PATE", "PATH", "PAVE",  "PAW", "PAWN",  "PAX",  "PAY", 
  "PEA" , "PEAK", "PEAL", "PEAR", "PEAT", "PECK", "PEEL", "PEEN", 
  "PEER", "PELT",  "PEN", "PEND", "PENT",  "PEP",  "PER", "PERK", 
  "PEST",  "PET",  "PEW",  "PHI",   "PI", "PICK",  "PIE", "PIER", 
  "PIG" , "PIKE", "PILE", "PILL",  "PIN", "PINE", "PING", "PINT", 
  "PIPE",  "PIT", "PITY", "PLAN", "PLAY", "PLEA", "PLED", "PLOD", 
  "PLOP", "PLOT", "PLOW", "PLOY", "PLUG", "PLUM", "PLUS",  "PLY", 
  "POD" , "POEM", "POET", "POKE", "POLE", "POLL", "POLO", "POMP", 
  "POND", "PONY", "POOF", "POOL", "POOR",  "POP", "PORE", "PORK", 
  "PORT", "POSE", "POSH", "POST",  "POT", "POUR",  "POW",  "POX", 
  "POXY", "PRAM", "PRAT", "PRAY", "PREP", "PREY", "PRIG", "PRIM", 
  "PROP",  "PRY",  "PUB", "PUCE", "PUCK", "PUFF",  "PUG", "PULL", 
  "PULP", "PUMP",  "PUN", "PUNK", "PUNT", "PUNY",  "PUP", "PURE", 
  "PURR", "PUSH",  "PUT", "PUTT", "QUAD", "QUIP", "QUIT", "QUIZ", 
  "QUO" , "RACE", "RAFT", "RAID", "RAIL", "RAIN", "RAKE",  "RAM", 
  "RAMP",  "RAN", "RANG", "RANK",  "RAP", "RAPT", "RARE", "RASH", 
  "RASP",  "RAT", "RATE", "RATH", "RAVE",  "RAW",  "RAY", "RAZE", 
  "RAZZ", "READ", "REAK", "REAL", "REAM", "REAP", "REAR",  "RED", 
  "REDO", "REED", "REEF", "REEL", "REIN", "REND", "RENT", "REST", 
  "REV" ,  "RIB", "RICE", "RICH", "RICK",  "RID", "RIDE", "RIFE", 
  "RIFF", "RIFT",  "RIG", "RILE",  "RIM", "RIND", "RING", "RINK", 
  "RIOT",  "RIP", "RIPE", "RISE", "RISK", "RITE", "ROAD", "ROAM", 
  "ROAR", "ROBE", "ROCK",  "ROD", "RODE",  "ROE", "ROIL", "ROLL", 
  "ROME", "ROOF", "ROOM", "ROOT", "ROPE", "ROSE", "ROSY",  "ROT", 
  "ROUT", "ROVE",  "ROW", "ROWS",  "RUB", "RUBY",  "RUG", "RUIN", 
  "RULE",  "RUM",  "RUN", "RUNE", "RUNG", "RUNS", "RUNT", "RUSE", 
  "RUSH", "RUST",  "RUT",  "RYE", "SACK",  "SAD", "SAFE", "SAGA", 
  "SAGE", "SAID", "SAIL", "SAKE", "SALE", "SALT", "SAME", "SAND", 
  "SANE", "SANG", "SANK", "SANS",  "SAP", "SASH",  "SAT", "SATE", 
  "SAVE",  "SAW", "SAWN",  "SAX",  "SAY", "SAYS", "SCAD", "SCAM", 
  "SCAN", "SCAR", "SCUM",  "SEA", "SEAL", "SEAM", "SEAR", "SEAT", 
  "SEE" , "SEED", "SEEK", "SEEM", "SEEN", "SEEP", "SEES", "SELF", 
  "SELL", "SEND", "SENT",  "SET", "SETS",  "SEW", "SEWN", "SHAW", 
  "SHE" , "SHED", "SHIM", "SHIN", "SHIP", "SHOD", "SHOE", "SHOO", 
  "SHOP", "SHOT", "SHOW", "SHUN", "SHUT",  "SHY",  "SIC", "SICK", 
  "SIDE", "SIFT", "SIGH", "SIGN", "SILK", "SILL", "SILO", "SILT", 
  "SIN" , "SINE", "SING", "SINK",  "SIR", "SIRE",  "SIS",  "SIT", 
  "SITE", "SITH", "SITS", "SITU",  "SIX", "SIZE", "SKEW",  "SKI", 
  "SKID", "SKIM", "SKIN", "SKIP", "SKIT",  "SKY", "SLAB", "SLAG", 
  "SLAM", "SLAP", "SLAT", "SLAW", "SLAY", "SLED", "SLEW", "SLID", 
  "SLIM", "SLIP", "SLOG", "SLOP", "SLOT", "SLOW", "SLUG", "SLUM", 
  "SLUR",  "SLY", "SMOG", "SMUG", "SNAG", "SNAP", "SNOB", "SNOT", 
  "SNOW", "SNUB", "SNUG",   "SO", "SOAK", "SOAP", "SOAR",  "SOB", 
  "SOCK",  "SOD", "SODA", "SOFA", "SOFT", "SOHO", "SOIL", "SOLD", 
  "SOLE", "SOLO", "SOME",  "SON", "SONG", "SONS", "SOON", "SOOT", 
  "SOP" , "SOPE", "SORE", "SORT", "SORY", "SOUL", "SOUP", "SOUR", 
  "SOW" , "SOWN",  "SOY",  "SPA", "SPAN", "SPAR", "SPAT", "SPAY", 
  "SPEC", "SPED", "SPEW", "SPIN", "SPIT", "SPOT", "SPRY", "SPUD", 
  "SPUR",  "SPY", "STAB", "STAG", "STAR", "STAY", "STEM", "STEP", 
  "STEW", "STIM", "STIR", "STOP", "STOW", "STUB", "STUD", "STUN", 
  "SUB" , "SUCH", "SUDS",  "SUE", "SUET", "SUIT", "SULK",  "SUM", 
  "SUMP", "SUMS",  "SUN", "SUNG", "SUNK", "SURE", "SURF", "SWAB", 
  "SWAD", "SWAG", "SWAM", "SWAN", "SWAP", "SWAT", "SWAY", "SWIG", 
  "SWIM", "SWUM", "SYNC",  "TAB", "TACK", "TACT",  "TAD",  "TAG", 
  "TAIL", "TAKE", "TALC", "TALE", "TALK", "TALL", "TAME", "TAMP", 
  "TAN" , "TANG", "TANK",  "TAP", "TAPE", "TAPS",  "TAR", "TARE", 
  "TARP", "TART", "TASK", "TAUT",  "TAX", "TAXI",  "TEA", "TEAK", 
  "TEAL", "TEAM", "TEAR", "TECH",  "TEE", "TEEM", "TEEN", "TELL", 
  "TEMP",  "TEN", "TEND", "TENT", "TERM", "TEST", "TEXT", "THAN", 
  "THAT", "THAW",  "THE", "THEE", "THEM", "THEN", "THEY", "THIN", 
  "THIS", "THOU", "THOW", "THUD",  "THY",  "TIC", "TICK", "TIDE", 
  "TIDY",  "TIE", "TIED", "TIER", "TIFF", "TILE", "TILL", "TILT", 
  "TIME",  "TIN", "TINE", "TINT", "TINY",  "TIP", "TIRE",   "TO", 
  "TOAD",  "TOE", "TOFU",  "TOG", "TOGA", "TOGS", "TOIL", "TOKE", 
  "TOLD", "TOLE", "TOLL", "TOMB",  "TON", "TONE", "TONG", "TONY", 
  "TOO" , "TOOK", "TOOL", "TOOT",  "TOP", "TOPS", "TORE", "TORN", 
  "TORT", "TOTE", "TOTO", "TOUR", "TOUT",  "TOW", "TOWN",  "TOY", 
  "TRAM", "TRAP", "TRAY", "TREE", "TREK", "TRIM", "TRIO", "TROD", 
  "TROT", "TRUE",  "TRY",  "TUB", "TUBA", "TUBE", "TUCK", "TUFT", 
  "TUG" , "TULE", "TUNA", "TUNE", "TURF", "TURN", "TUSK",  "TUT", 
  "TUTU",  "TUX", "TWAS", "TWEE", "TWIG", "TWIN", "TWIT",  "TWO", 
  "TYPE", "TYPO", "UGLY", "UNDO", "UNIT", "UNTO",   "UP", "UPON", 
  "URGE",  "URN",   "US",  "USE", "USED", "USER", "USES", "VAIN", 
  "VALE",  "VAN", "VANE", "VARY", "VASE", "VAST",  "VAT", "VEAL", 
  "VEER", "VEIL", "VEIN", "VEND", "VENT", "VERB", "VERY", "VEST", 
  "VET" , "VETO",  "VEX",  "VIA", "VICE",  "VIE", "VIEW", "VILA", 
  "VINE", "VISA", "VISE", "VOID", "VOLT", "VOTE",  "VOW",  "WAD", 
  "WADE", "WAFT",  "WAG", "WAGE", "WAIK", "WAIL", "WAIT", "WAKE", 
  "WALK", "WALL", "WAND", "WANE", "WANT",  "WAR", "WARD", "WARE", 
  "WARM", "WARN", "WARP", "WART", "WARY",  "WAS", "WASH", "WASP", 
  "WATT", "WAVE", "WAVY",  "WAX", "WAXY",  "WAY", "WAYS",   "WE", 
  "WEAK", "WEAL", "WEAN", "WEAR",  "WEB",  "WED",  "WEE", "WEED", 
  "WEEK", "WEEP", "WEIR", "WELD", "WELL", "WELT", "WEND", "WENT", 
  "WEPT", "WERE", "WEST",  "WET", "WEVE", "WHAM", "WHAT", "WHEN", 
  "WHET", "WHEY", "WHIM", "WHIP", "WHIZ",  "WHO", "WHOA", "WHOM", 
  "WHY" , "WICK", "WIDE", "WIFE",  "WIG", "WILD", "WILL", "WILT", 
  "WILY",  "WIN", "WIND", "WINE", "WING", "WINK", "WIPE", "WIRE", 
  "WISE", "WISH", "WISP",  "WIT", "WITH",  "WOK", "WOKE", "WOLF", 
  "WON" , "WONT",  "WOO", "WOOD", "WOOL", "WORD", "WORE", "WORK", 
  "WORM", "WORN", "WORT",  "WOT",  "WOW", "WRAP", "WREN", "WRIT", 
  "WRY" , "WUSS", "YAGI",  "YAK",  "YAM", "YANK",  "YAP", "YARD", 
  "YARN",  "YAW", "YAWN",  "YEA", "YEAH", "YEAR", "YELL", "YELP", 
  "YES" ,  "YET", "YOGA", "YOGI", "YOKE", "YOLK", "YORE", "YORK", 
  "YOU" , "YOUD", "YOUR", "YOWL", "YURT", "ZERO", "ZEST", "ZETA",
  "ZINC", "ZING", "ZINK",  "ZIP", "ZONE",  "ZOO", "ZORK", "ZOOM"

};

//
void
banner(FILE* stream = stdout)
{
  fprintf(stream, "\n\
%s (asdcplib %s)\n\n\
Copyright (c) 2003-2018 John Hurst\n\n\
%s is part of the asdcp DCP tools package.\n\
asdcplib may be copied only under the terms of the license found at\n\
the top of every file in the asdcplib distribution kit.\n\n\
Specify the -h (help) option for further information about %s\n\n",
	  PROGRAM_NAME, Kumu::Version(), PROGRAM_NAME, PROGRAM_NAME);
}

//
void
usage(FILE* stream = stdout)
{
  fprintf(stream, "\
USAGE: %s [-b|-B|-c|-x] [-n] [-s <size>] [-v]\n\
\n\
       %s [-h|-help] [-V]\n\
\n\
  -b          - Output a stream of binary data\n\
  -B          - Output a Base64 string\n\
  -c          - Output a C-language struct containing the values\n\
  -C          - Encode as a list of code words (for each code word, 20 bits of\n\
                entropy are consumed, 4 are discarded)\n\
  -h | -help  - Show help\n\
  -n          - Suppress newlines\n\
  -s <size>   - Number of random bytes to generate (default 32)\n\
  -v          - Verbose. Prints informative messages to stderr\n\
  -V          - Show version information\n\
  -w          - Encode as a list of dictionary words (for each dictionary word,\n\
                11 bits of entropy are consumed, 1 is discarded)\n\
  -W <expr>   - Word separator, for use with -C and -w (default '-')\n\
  -x          - Output hexadecimal (default)\n\
\n\
  NOTES: o There is no option grouping, all options must be distinct arguments.\n\
         o All option arguments must be separated from the option by whitespace.\n\
\n", PROGRAM_NAME, PROGRAM_NAME);
}

enum OutputFormat_t {
  OF_HEX,
  OF_BINARY,
  OF_BASE64,
  OF_CSTRUCT,
  OF_DICTWORD,
  OF_CODEWORD
};

//
class CommandOptions
{
  CommandOptions();

public:
  bool   error_flag;      // true if the given options are in error or not complete
  bool   no_newline_flag; // 
  bool   verbose_flag;    // true if the verbose option was selected
  bool   version_flag;    // true if the version display option was selected
  bool   help_flag;       // true if the help display option was selected
  OutputFormat_t format;  // 
  ui32_t request_size;
  bool size_provided;     // if true, the -s option has been used
  std::string separator;  // word separator value for OF_DICTWORD and CODEWORD modes

 //
  CommandOptions(int argc, const char** argv) :
    error_flag(true), no_newline_flag(false), verbose_flag(false),
    version_flag(false), help_flag(false), format(OF_HEX), request_size(RandBlockSize*2),
    size_provided(false), separator("-")
  {
    for ( int i = 1; i < argc; i++ )
      {

	 if ( (strcmp( argv[i], "-help") == 0) )
	   {
	     help_flag = true;
	     continue;
	   }
     
	if ( argv[i][0] == '-' && isalpha(argv[i][1]) && argv[i][2] == 0 )
	  {
	    switch ( argv[i][1] )
	      {
	      case 'b': format = OF_BINARY; break;
	      case 'B': format = OF_BASE64; break;
	      case 'c': format = OF_CSTRUCT; break;
	      case 'C': format = OF_CODEWORD; break;
	      case 'n': no_newline_flag = true; break;
	      case 'h': help_flag = true; break;

	      case 's':
		TEST_EXTRA_ARG(i, 's');
		request_size = Kumu::xabs(strtol(argv[i], 0, 10));
		size_provided = true;
		break;

	      case 'v': verbose_flag = true; break;
	      case 'V': version_flag = true; break;
	      case 'w': format = OF_DICTWORD; break;

	      case 'W':
		TEST_EXTRA_ARG(i, 'W');
		separator = argv[i];
		break;

	      case 'x': format = OF_HEX; break;

	      default:
		fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
		return;
	      }
	  }
	else
	  {
	    fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
	    return;
	  }
      }

    if ( help_flag || version_flag )
      return;

    if ( ! size_provided )
      {
	if ( format == OF_CODEWORD )
	  {
	    request_size = 20;
	  }
	else if ( format == OF_DICTWORD )
	  {
	    request_size = 16;
	  }
      }
    else if ( request_size == 0 )
      {
	fprintf(stderr, "Please use a non-zero request size\n");
	return;
      }

    error_flag = false;
  }
};


//
int
main(int argc, const char** argv)
{
  CommandOptions Options(argc, argv);

   if ( Options.version_flag )
    banner();

  if ( Options.help_flag )
    usage();

  if ( Options.version_flag || Options.help_flag )
    return 0;

  if ( Options.error_flag )
    {
      fprintf(stderr, "There was a problem. Type %s -h for help.\n", PROGRAM_NAME);
      return 3;
    }

  FortunaRNG    RandGen;
  ByteString    Buf(Kumu::Kilobyte);

  if ( Options.verbose_flag )
    fprintf(stderr, "Generating %d random byte%s.\n", Options.request_size, (Options.request_size == 1 ? "" : "s"));

  if ( Options.format == OF_BINARY )
    {
      if ( KM_FAILURE(Buf.Capacity(Options.request_size)) )
	{
	  fprintf(stderr, "randbuf: %s\n", RESULT_ALLOC.Label());
	  return 1;
	}

      RandGen.FillRandom(Buf.Data(), Options.request_size);
      fwrite((byte_t*)Buf.Data(), 1, Options.request_size, stdout);
    }
  else if ( Options.format == OF_CSTRUCT )
    {
      ui32_t line_count = 0;
      byte_t* p = Buf.Data();
      printf("byte_t rand_buf[%u] = {\n", Options.request_size);

      if ( Options.request_size > 128 )
	fputs("  // 0x00000000\n", stdout);

      while ( Options.request_size > 0 )
	{
	  if ( line_count > 0 && (line_count % (RandBlockSize*8)) == 0 )
	    fprintf(stdout, "  // 0x%08x\n", line_count);

	  RandGen.FillRandom(p, RandBlockSize);
	  fputc(' ', stdout);

	  for ( ui32_t i = 0; i < RandBlockSize && Options.request_size > 0; i++, Options.request_size-- )
	    printf(" 0x%02x,", p[i]);

	  fputc('\n', stdout);
	  line_count += RandBlockSize;
	}

      fputs("};", stdout);

      if ( ! Options.no_newline_flag )
	fputc('\n', stdout);
    }
  else if ( Options.format == OF_BASE64 )
    {
      if ( KM_FAILURE(Buf.Capacity(Options.request_size)) )
	{
	  fprintf(stderr, "randbuf: %s\n", RESULT_ALLOC.Label());
	  return 1;
	}

      ByteString Strbuf;
      ui32_t e_len = base64_encode_length(Options.request_size) + 1;

      if ( KM_FAILURE(Strbuf.Capacity(e_len)) )
        {
          fprintf(stderr, "strbuf: %s\n", RESULT_ALLOC.Label());
          return 1;
        }

      RandGen.FillRandom(Buf.Data(), Options.request_size);

      if ( base64encode(Buf.RoData(), Options.request_size, (char*)Strbuf.Data(), Strbuf.Capacity()) == 0 )
	{
          fprintf(stderr, "encode error\n");
          return 2;
        } 

      fputs((const char*)Strbuf.RoData(), stdout);

      if ( ! Options.no_newline_flag )
	fputs("\n", stdout);
    }
  else if ( Options.format == OF_DICTWORD )
    {
      byte_t* p = Buf.Data();
      char hex_buf[64];
      int word_count = 0;

      while ( Options.request_size > 0 )
	{
	  ui32_t x_len = xmin(Options.request_size, RandBlockSize);
	  RandGen.FillRandom(p, RandBlockSize);

	  // process 3 words at a time, each containing two 12-bit segments
	  for ( int i = 0; i+3 < x_len; i+=3 )
	    {
	      // ignore the high bit of each 12-bit segment, thus each output
	      // word has 11 bits of the entropy
	      int index1 = ( ( p[i] & 0x07f ) << 4 ) | ( p[i+1] >> 4 );
	      assert(index1<2048);
	      int index2 = ( ( p[i+1] & 0x07 ) << 8 ) | p[i+2];
	      assert(index2<2048);

	      printf("%s%s%s%s",
		     (word_count==0?"":Options.separator.c_str()),
		     _word_list[index1],
		     Options.separator.c_str(),
		     _word_list[index2]);

	      word_count += 2;
	    }

	  Options.request_size -= x_len;
	}

      if ( ! Options.no_newline_flag )
	{
	  fputc('\n', stdout);
	}
    }
  else if ( Options.format == OF_CODEWORD )
    {
      const char word_chars[] = "123456789ABCDEFGHJKLMNPRSTUVWXYZ";
      byte_t* p = Buf.Data();
      char hex_buf[64];
      int word_count = 0;

      while ( Options.request_size > 0 )
	{
	  ui32_t x_len = xmin(Options.request_size, RandBlockSize);
	  RandGen.FillRandom(p, RandBlockSize);

	  // process 3 words at a time, derive a code word with 20 bits of entropy.
	  for ( int i = 0; i+3 < x_len; i+=3 )
	    {
	      int index1 = p[i] >> 3;                 // MSB bits 87654
	      assert(index1<32);
	      int index2 = ( ( p[i]  & 0x07 ) << 2 ) | ( p[i+1] >> 6 );    // MSB bits 321 & MSB+1 bits 78
	      assert(index2<32);
	      int index3 = ( p[i+1] & 0x3e ) >> 1;    // MSB+1 bits 65432
	      assert(index3<32);
	      int index4 = p[i+2]  & 0x1f; 	      // MSB+2 bits 54321
	      assert(index4<32);

	      printf("%s%c%c%c%c",
		     (word_count==0?"":Options.separator.c_str()),
		     word_chars[index1], word_chars[index2],
		     word_chars[index3], word_chars[index4]);

	      ++word_count;
	    }

	  Options.request_size -= x_len;
        }

      if ( ! Options.no_newline_flag )
	{
	  fputc('\n', stdout);
	}
    }
 else // OF_HEX
    {
      byte_t* p = Buf.Data();
      char hex_buf[64];

      while ( Options.request_size > 0 )
	{
	  ui32_t x_len = xmin(Options.request_size, RandBlockSize);
	  RandGen.FillRandom(p, RandBlockSize);
	  bin2hex(p, x_len, hex_buf, 64);
          fputs(hex_buf, stdout);

	  Options.request_size -= x_len;

          if ( ! Options.no_newline_flag )
	    {
	      fputc('\n', stdout);
	    }
	}
    }

  return 0;
}


//
// end kmrandgen.cpp
//
