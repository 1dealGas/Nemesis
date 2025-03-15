// Nemesis, the Aerials Fumen Compiler. //
#ifndef AR_BUILD_VIEWER
#include <algorithm>
#include <Arf4.h>
#include <map>

/* Internal Typedefs & Fns */
namespace N4 {
	struct Tempo {
		double		bar, beatBase, toneBase;
		uint32_t	a, b;   // Tempo a/b
	};
	struct Delta {
		double		init, value;
		double		base;
	};

	/* Wish */
	struct Point {
		double		x, y, beat, radius, degree;
		uint8_t		ease;
	};
	struct Child {
		double		beat;
		double		radius, initLoop, deltaLoop;
	};

	/* Object */
	struct Hint {
		double		beat;
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
		//------------------------//
		float				wRadius;
		double				wX, wY, wNx, wNy, wDegree;
	};
	struct Build {
		std::vector<Tempo>	tempoList = {{0,4,4}};
		std::vector<Delta>	bpmList = {{0,170}}, deltas = {{0,1}};
		std::vector<Wish>	wishes;
		std::vector<Echo>	echoes;
		uint64_t			verseWidx:14, verseEidx:15;
		uint64_t			tIdx:11, bIdx:11, dIdx:13;
		double				sinceTone;
	};
}

static N4::Build N;
static double barToTone(const double bar) noexcept {
	if( const auto lastTempo = N.tempoList.back();  bar >= lastTempo.bar )
		return lastTempo.toneBase + (bar - lastTempo.bar) * lastTempo.a / lastTempo.b;

	const auto initIt = N.tempoList.begin(), lastIt = N.tempoList.end() - 1;
		  auto it = initIt + N.tIdx;
	if( it != initIt  &&  bar < it->bar )
		do	 --it;
		while( it != initIt  &&  bar < it->bar );
	/**/auto nextIt = it + 1;
	while( it != lastIt  &&  bar >= nextIt->bar )
		++it, ++nextIt;
	N.tIdx = it - initIt;

	const auto thiz = *it;
	return thiz.toneBase + fabs(bar - thiz.bar) * thiz.a / thiz.b;
}

static double toneToBar(const double tone) noexcept {
	if( const auto lastTempo = N.tempoList.back();  tone >= lastTempo.toneBase )
		return lastTempo.bar + (tone - lastTempo.toneBase) * lastTempo.b / lastTempo.a;

	const auto initIt = N.tempoList.begin(), lastIt = N.tempoList.end() - 1;
		  auto it = initIt + N.tIdx;
	if( it != initIt  &&  tone < it->toneBase )
		do	 --it;
		while( it != initIt  &&  tone < it->toneBase );
	/**/auto nextIt = it + 1;
	while( it != lastIt  &&  tone >= nextIt->toneBase )
		++it, ++nextIt;
	N.tIdx = it - initIt;

	const auto thiz = *it;
	return thiz.bar + fabs(tone - thiz.toneBase) * thiz.b / thiz.a;
}

static double barToBeat(const double bar) noexcept {
	if( const auto lastTempo = N.tempoList.back();  bar >= lastTempo.bar )
		return lastTempo.toneBase + (bar - lastTempo.bar) * lastTempo.a;

	const auto initIt = N.tempoList.begin(), lastIt = N.tempoList.end() - 1;
		  auto it = initIt + N.tIdx;
	if( it != initIt  &&  bar < it->bar )
		do	 --it;
		while( it != initIt  &&  bar < it->bar );
	/**/auto nextIt = it + 1;
	while( it != lastIt  &&  bar >= nextIt->bar )
		++it, ++nextIt;
	N.tIdx = it - initIt;

	const auto thiz = *it;
	return thiz.toneBase + fabs(bar - thiz.bar) * thiz.a;
}

static double beatToMs(const double beat) noexcept {
	if( const auto lastBpm = N.bpmList.back();  beat >= lastBpm.init )
		return lastBpm.base + (beat - lastBpm.init) * (60000 / lastBpm.value);

	const auto initIt = N.bpmList.begin(), lastIt = N.bpmList.end() - 1;
		  auto it = initIt + N.bIdx;
	if( it != initIt  &&  beat < it->init )
		do	 --it;
		while( it != initIt  &&  beat < it->init );
	/**/auto nextIt = it + 1;
	while( it != lastIt  &&  beat >= nextIt->init )
		++it, ++nextIt;
	N.bIdx = it - initIt;

	const auto thiz = *it;
	return thiz.base + fabs(beat - thiz.init) * (60000 / thiz.value);
}

static double msToDt(const double ms) noexcept {
	if( const auto lastDelta = N.deltas.back();  ms >= lastDelta.init )
		return lastDelta.base + (ms - lastDelta.init) * lastDelta.value;

	const auto initIt = N.deltas.begin(), lastIt = N.deltas.end() - 1;
		  auto it = initIt + N.dIdx;
	if( it != initIt  &&  ms < it->init )
		do	 --it;
		while( it != initIt  &&  ms < it->init );
	/**/auto nextIt = it + 1;
	while( it != lastIt  &&  ms >= nextIt->init )
		++it, ++nextIt;
	N.dIdx = it - initIt;

	const auto thiz = *it;
	return thiz.base + (ms - thiz.init) * thiz.value;
}

static double nextDouble(const double d) noexcept {   // Little Endian Only
	union{ double d; uint64_t u; }  nd = {d};
	return ++nd.u, nd.d;
}

static bool isSimilarDouble(const double a, const double b) noexcept {   // Internal, a < b required
	const union{ double d; uint64_t u; }  A = {a}, B = {b};
	return  B.u - A.u < 2;
}

static void wishCacheT(N4::Wish& w, const double beat) noexcept {
	if( const auto& first = w.nodes.front();  beat < first.beat )
		w.wNx = first.x, w.wNy = first.y, w.wRadius = first.radius, w.wDegree = first.degree;
	else if( const auto& last = w.nodes.back();  beat >= last. beat )
		w.wNx = last.x, w.wNy = last.y, w.wRadius = last.radius, w.wDegree = last.degree;
	else {
		std::vector<N4::Point>::const_iterator thiz, next;
		if( thiz = w.nodes.cbegin()+w.nIdx,  beat < thiz->beat ) {
			do   --w.nIdx;
			while( --thiz,  beat < thiz->beat );
			next = thiz + 1;
		}
		else if( next = thiz+1,  beat >= next->beat ) {
			do   ++w.nIdx;
			while( ++next,  beat >= next->beat );
			thiz = next - 1;
		}
		const double ratio = Ar::Eased( (beat - thiz->beat) / (next->beat - thiz->beat), thiz->ease );
		w.wNx = thiz->x + (next->x - thiz->x) * ratio;
		w.wNy = thiz->y + (next->y - thiz->y) * ratio;
		w.wRadius = thiz->radius + (next->radius - thiz->radius) * ratio;
		w.wDegree = thiz->degree + (next->degree - thiz->degree) * ratio;
	}
	const auto cosSin = Ar::CosSin({ .a = (float)w.wDegree });
	w.wX = w.wNx + w.wRadius * cosSin.a;
	w.wY = w.wNy + w.wRadius * cosSin.b;
}

static double checkTime(lua_State* L, const int where) noexcept {
	// WIP
}

static int wishGetActualX(lua_State* L) noexcept {
	N4::Wish* w;
	if( lua_type(L, 1) == LUA_TNUMBER )
		w = (N4::Wish*)lua_touserdata(L, 2), wishCacheT( *w, checkTime(L, 1) );
	else
		w = (N4::Wish*)lua_touserdata(L, 1), wishCacheT( *w, checkTime(L, 2) );
	lua_pushnumber(L, w->wX);
	return 1;
}

static int wishGetActualY(lua_State* L) {
	N4::Wish* w;
	if( lua_type(L, 1) == LUA_TNUMBER )
		w = (N4::Wish*)lua_touserdata(L, 2), wishCacheT( *w, checkTime(L, 1) );
	else
		w = (N4::Wish*)lua_touserdata(L, 1), wishCacheT( *w, checkTime(L, 2) );
	lua_pushnumber(L, w->wY);
	return 1;
}

static int wishGetInfo(lua_State* L) {
	const auto w = (N4::Wish*)lua_touserdata(L, 1);
	wishCacheT( *w, checkTime(L, 2) );
	lua_createtable(L, 6, 0);

	return
		lua_pushnumber(L, w->wX),		lua_rawseti(L, -2, 1),
		lua_pushnumber(L, w->wY),		lua_rawseti(L, -2, 2),
		lua_pushnumber(L, w->wNx),		lua_rawseti(L, -2, 3),
		lua_pushnumber(L, w->wNy),		lua_rawseti(L, -2, 4),
		lua_pushnumber(L, w->wRadius),	lua_rawseti(L, -2, 5),
		lua_pushnumber(L, w->wDegree),	lua_rawseti(L, -2, 6),
	1;
}

static int freeHelper(lua_State* L) noexcept {
	( (N4::Wish*)lua_touserdata(L, 1) ) -> ~Wish();
	return 0;
}


/* Script APIs */
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