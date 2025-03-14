// Nemesis, the Aerials Fumen Compiler. //
#ifdef AR_BUILD_VIEWER
#include <algorithm>
#include <Arf4.h>
#include <map>

/* Internal Typedefs & Fns */
namespace N4 {
	struct Tempo {
		double		bar, beatBase, toneBase;
		uint32_t	beatPbar, toneDivisor;
	};
	struct Delta {
		double		beat, base;
		double		value;
	};

	/* Wish */
	struct Point {
		double		x, y, beat, radius, degree;
		uint8_t		ease;
	};
	struct Child {
		double		deltaBeat;
		double		radius, initLoop, deltaLoop;
	};

	/* Object */
	struct Hint {
		double		deltaBeat;
		bool		isSpecial;
	};
	struct Echo {
		double		x, y, beat;
		double		radius, initLoop, deltaLoop;
		bool		isSpecial;
	};

	/* Build */
	struct Wish {
		std::vector<Point>	nodes;
		std::vector<Child>	wishChilds;
		std::vector<Hint>	manualHints;
		uint8_t				nIdx, isSpecial, withDt;

	};
	struct Build {
		std::vector<Tempo>	tempoList;
		std::vector<Delta>	bpmList, deltas;
		std::vector<Wish>	wishes;
		std::vector<Echo>	echoes;
		uint16_t			verseWidx, verseEidx, tIdx, bIdx;
		double				sinceTone;
	};
}

static double barToTone(const double bar) {

}

static double toneToBar(const double tone) {

}

static double barToBeat(const double bar) {

}

static double beatToMs(const double beat) {

}

static double barToDt(const double beat) {

}

static N4::Point checkInfo(lua_State* L, const int idx, const int n, const float t) {

}

static int wishGetActualX(lua_State* L) {

}

static int wishGetActualY(lua_State* L) {

}

static int wishGetInfo(lua_State* L) {

}

static int freeHelper(lua_State* L) {

}


/* Script APIs */
static N4::Build N;
int Ar::NewBuild(lua_State* L) {}
int Ar::SetDelta(lua_State* L) noexcept {}

int Ar::NewWish(lua_State* L) {}
int Ar::NewHint(lua_State* L) {}
int Ar::NewEcho(lua_State* L) {}
int Ar::NewChild(lua_State* L) {}
int Ar::NewHelper(lua_State* L) noexcept {}

int Ar::NewVerse(lua_State*) noexcept {}
int Ar::Mirror(lua_State*) noexcept {}

int Ar::DeltaTone(lua_State* L) noexcept {}
int Ar::BarToMs(lua_State* L) noexcept {}


/* Arf Compiler */
int Ar::OrganizeArf(lua_State* L) noexcept {

}
#endif