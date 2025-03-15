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
		std::vector<Tempo>	tempoList = {{ 0,4,4 }};
		std::vector<Delta>	beatToMs = {{ 0, 60000/170.0 }},  deltas = {{ 0,1 }};
		std::vector<Wish>	wishes;
		std::vector<Echo>	echoes;
		uint64_t			verseWidx:14, verseEidx:15;
		uint64_t			tIdx:11, bIdx:11, dIdx:13;
		double				sinceTone;
	};
}

static N4::Build N;
static double barToTone(const double bar) noexcept {   // With User Input
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
	return thiz.toneBase + fmax(bar - thiz.bar, 0) * thiz.a / thiz.b;
}

static double toneToBar(const double tone) noexcept {   // With User Input
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
	return thiz.bar + fmax(tone - thiz.toneBase, 0) * thiz.b / thiz.a;
}

static double barToBeat(const double bar) noexcept {   // Internal, bar >= 0 required
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
	return thiz.toneBase + (bar - thiz.bar) * thiz.a;
}

static double beatToMs(const double beat) noexcept {   // Internal, beat >= 0 required
	if( const auto lastBpm = N.beatToMs.back();  beat >= lastBpm.init )
		return lastBpm.base + (beat - lastBpm.init) * lastBpm.value;

	const auto initIt = N.beatToMs.begin(), lastIt = N.beatToMs.end() - 1;
		  auto it = initIt + N.bIdx;
	if( it != initIt  &&  beat < it->init )
		do	 --it;
		while( it != initIt  &&  beat < it->init );
	/**/auto nextIt = it + 1;
	while( it != lastIt  &&  beat >= nextIt->init )
		++it, ++nextIt;
	N.bIdx = it - initIt;

	const auto thiz = *it;
	return thiz.base + (beat - thiz.init) * thiz.value;
}

static double msToDt(const double ms) noexcept {   // Internal, ms >= 0 required
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

static double checkTime(lua_State* L, const int where) {
	if( double T;  lua_type(L, where) == LUA_TTABLE )
		return lua_rawgeti(L, where, 1),  lua_rawgeti(L, where, 2),   // [-2] sinceBar  [-1] withTone
			   N.sinceTone = barToTone( lua_tonumber(L, -2) ),  T = lua_tonumber(L, -1),  lua_pop(L, 2),
			   barToBeat(toneToBar(  N.sinceTone + ( T<0 ? -T/16 : T )  ));
	else
		return T = lua_tonumber(L, where),
			   barToBeat(toneToBar(  N.sinceTone + ( T<0 ? -T/16 : T )  ));
}

static double wishCheckTime(lua_State* L, const int where) {
	if( double S, T;  lua_type(L, where) == LUA_TTABLE )
		return lua_rawgeti(L, where, 1),  lua_rawgeti(L, where, 2),   // [-2] sinceBar  [-1] withTone
			   S = barToTone( lua_tonumber(L, -2) ),  T = lua_tonumber(L, -1),  lua_pop(L, 2),
			   barToBeat(toneToBar(  S + ( T<0 ? -T/16 : T )  ));
	else
		return T = lua_tonumber(L, where),
			   barToBeat(toneToBar(  N.sinceTone + ( T<0 ? -T/16 : T )  ));
}

static int wishGetActualX(lua_State* L) noexcept {
	N4::Wish* w;
	if( lua_type(L, 1) == LUA_TUSERDATA )
		w = (N4::Wish*)lua_touserdata(L, 1), wishCacheT( *w, wishCheckTime(L, 2) );
	else
		w = (N4::Wish*)lua_touserdata(L, 2), wishCacheT( *w, wishCheckTime(L, 1) );
	lua_pushnumber(L, w->wX);
	return 1;
}

static int wishGetActualY(lua_State* L) noexcept {
	N4::Wish* w;
	if( lua_type(L, 1) == LUA_TUSERDATA )
		w = (N4::Wish*)lua_touserdata(L, 1), wishCacheT( *w, wishCheckTime(L, 2) );
	else
		w = (N4::Wish*)lua_touserdata(L, 2), wishCacheT( *w, wishCheckTime(L, 1) );
	lua_pushnumber(L, w->wY);
	return 1;
}

static int wishGetInfo(lua_State* L) noexcept {
	const auto w = (N4::Wish*)lua_touserdata(L, 1);
	wishCacheT( *w, wishCheckTime(L, 2) );
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
static std::map<double, N4::Delta> bpmMap;
static std::map<double, N4::Tempo> tempoMap;
int Ar::NewBuild(lua_State* L)  /* Exception thrown by Lua */  {
	/* Usage:
	 * Time {					-- For 4/4-only tracks
	 *     Offset = 0,			-- Beat 0 starts from 0ms
	 *     0, 170,				-- Bar, BPM
	 *     ···
	 * }
	 * Time {					-- For tracks with Tempo Variations
	 *     Offset = 0,			-- Offset must be positive
	 *     Tempo = {
	 *         0, 4, 4,			-- Bar, Beat Count of a Bar, How many Beats are equal in length to an Tone
	 *         1, 3, 4,
	 *         25, 4, 4
	 *     },
	 *     0, 0, 201,			-- Bar(to be converted to Beat), Additional Beats, BPM
	 *     ···
	 * }
	 */
	const double offset = (lua_getfield(L, 1, "Offset"), fmax( lua_tonumber(L,-1), 0 ));
	lua_pop(L, 1);   // If succeeded, [1] must be a Table since then.
	N = {};

	// Tempo
	tempoMap.clear();
	lua_getfield(L, 1, "Tempo");
	if( size_t tempoInputLen;  lua_istable(L,-1)  &&  ( tempoInputLen = lua_objlen(L,-1) ) > 2 )
		for( size_t i = 1;  i < tempoInputLen;  i += 3 ) {   // [1] Args Table  [2] Tempo Table
			double bar = ( lua_rawgeti(L, 2, i), lua_tonumber(L, -1) );
				   bar = bar < 0 ? 0 : bar;
			const uint32_t a = ( lua_rawgeti(L, 2, i+1), lua_tointeger(L, -1) ),
						   b = ( lua_rawgeti(L, 2, i+2), lua_tointeger(L, -1) );
			tempoMap[bar] = { .bar = bar,  .a = a ? a : 4,  .b = b ? b : 4 };
			lua_pop(L, 3);
		}
	lua_pop(L, 1);

	// BPM Input
	bpmMap.clear();
	if( const size_t bpmInputLen = lua_objlen(L, 1);  tempoMap.empty() ) {
		for( size_t i = 1;  i < bpmInputLen;  i += 2 ) {
			const double beat = barToBeat(( lua_rawgeti(L,1,i), lua_tonumber(L,-1) ));   // >=0 Clamped
				  double bpm = ( lua_rawgeti(L, 1, i+1), lua_tonumber(L, -1) );
						 bpm = bpm > 0 ? bpm : 170;
			bpmMap[beat] = { .init = beat, .value = 60000 / bpm };
		}
	}
	else {
		const size_t tempoCount = tempoMap.size();
		N.tempoList.reserve(tempoCount);
		N.tempoList.clear();

		for( const auto [_, tempo] : tempoMap )
			N.tempoList.push_back(tempo);
		N.tempoList[0].bar = 0;   // In case firstTempo.bar > 0

		for( size_t i = 1;  i < tempoCount;  ++i ) {
			const auto  lastTempo = N.tempoList[i-1];
				  auto& thisTempo = N.tempoList[i];
			const double deltaBar = thisTempo.bar - lastTempo.bar;
			thisTempo.toneBase = lastTempo.toneBase + deltaBar * lastTempo.a / lastTempo.b;
			thisTempo.beatBase = lastTempo.beatBase + deltaBar * lastTempo.a;
		}
		for( size_t i = 1;  i < bpmInputLen;  i += 3 ) {
			double beat = barToBeat(( lua_rawgeti(L, 1, i), lua_tonumber(L, -1) ))
						+ ( lua_rawgeti(L, 1, i+1), lua_tonumber(L, -1) );
				   beat = beat < 0 ? 0 : beat;
			double bpm = ( lua_rawgeti(L, 1, i+2), lua_tonumber(L, -1) );
				   bpm = bpm > 0 ? bpm : 170;
			bpmMap[beat] = { .init = beat, .value = 60000 / bpm };
		}
	}

	// Organize BPMs
	switch( const size_t bpmCnt = bpmMap.size();  bpmCnt ) {
		case 0:
			N.beatToMs[0] = { .base = offset, .init = 0, .value = 60000 / 170.0 };
			break;
		case 1:
			N.beatToMs[0] = { .base = offset, .init = 0, .value = bpmMap.cbegin()->second.value };
			break;
		default:
			N.beatToMs.clear();
			N.beatToMs.reserve( bpmCnt );
			for( const auto [_, node] : bpmMap )
				N.beatToMs.push_back(node);
			N.beatToMs[0].init = 0;
			N.beatToMs[0].base = offset;

			for( size_t i = 1;  i < bpmCnt;  ++i ) {
				const auto  lastBpm = N.beatToMs[i-1];
					  auto& thisBpm = N.beatToMs[i];
				thisBpm.base = lastBpm.base + (thisBpm.init - lastBpm.init) * lastBpm.value;
			}
	}

	// Provide Metatables
	if( luaL_newmetatable(L, "NEMESIS_WISH") )
		lua_pushcfunction(L, wishGetActualX),	lua_setfield(L, -2, "__add"),
		lua_pushcfunction(L, wishGetActualY),	lua_setfield(L, -2, "__sub"),
		lua_pushcfunction(L, wishGetInfo),		lua_setfield(L, -2, "__call");
	if( luaL_newmetatable(L, "NEMESIS_HELPER") )
		lua_pushcfunction(L, wishGetActualX),	lua_setfield(L, -2, "__add"),
		lua_pushcfunction(L, wishGetActualY),	lua_setfield(L, -2, "__sub"),
		lua_pushcfunction(L, wishGetInfo),		lua_setfield(L, -2, "__call"),
		lua_pushcfunction(L, freeHelper),		lua_setfield(L, -2, "__gc");
	return 0;
}

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


/* Arf Compile Fn */
int Ar::OrganizeArf(lua_State* L) noexcept {

}
#endif