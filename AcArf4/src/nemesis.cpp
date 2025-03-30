// Nemesis, the Aerials Fumen Compiler. //
#ifdef AR_BUILD_VIEWER
#include <algorithm>
#include <Arf4.h>
#include <span>
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
		bool		hintSpecial;
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
		std::vector<Tempo>	tempoList = {{ .a=4, .b=4 }};
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
	while( it != initIt  &&  bar < it->bar ) { --it; }
	/**/auto nextIt = it + 1;
	while( it != lastIt  &&  bar >= nextIt->bar )
		++it, ++nextIt;
	N.tIdx = it - initIt;

	const auto thiz = *it;
	return thiz.toneBase + fmax(bar - thiz.bar, 0) * thiz.a / thiz.b;   // tone >= 0 guaranteed
}

static double toneToBar(const double tone) noexcept {   // Internal, tone >= 0 required
	if( const auto lastTempo = N.tempoList.back();  tone >= lastTempo.toneBase )
		return lastTempo.bar + (tone - lastTempo.toneBase) * lastTempo.b / lastTempo.a;

	const auto initIt = N.tempoList.begin(), lastIt = N.tempoList.end() - 1;
		  auto it = initIt + N.tIdx;
	while( it != initIt  &&  tone < it->toneBase ) { --it; }
	/**/auto nextIt = it + 1;
	while( it != lastIt  &&  tone >= nextIt->toneBase )
		++it, ++nextIt;
	N.tIdx = it - initIt;

	const auto thiz = *it;
	return thiz.bar + (tone - thiz.toneBase) * thiz.b / thiz.a;
}

static double barToBeat(const double bar) noexcept {   // Internal, bar >= 0 required
	if( const auto lastTempo = N.tempoList.back();  bar >= lastTempo.bar )
		return lastTempo.beatBase + (bar - lastTempo.bar) * lastTempo.a;

	const auto initIt = N.tempoList.begin(), lastIt = N.tempoList.end() - 1;
		  auto it = initIt + N.tIdx;
	while( it != initIt  &&  bar < it->bar ) { --it; }
	/**/auto nextIt = it + 1;
	while( it != lastIt  &&  bar >= nextIt->bar )
		++it, ++nextIt;
	N.tIdx = it - initIt;

	const auto thiz = *it;
	return thiz.beatBase + (bar - thiz.bar) * thiz.a;
}

static double beatToMs(const double beat) noexcept {   // Internal, beat >= 0 required
	if( const auto lastBpm = N.beatToMs.back();  beat >= lastBpm.init )
		return lastBpm.base + (beat - lastBpm.init) * lastBpm.value;

	const auto initIt = N.beatToMs.begin(), lastIt = N.beatToMs.end() - 1;
		  auto it = initIt + N.bIdx;
	while( it != initIt  &&  beat < it->init ) { --it; }
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
	while( it != initIt  &&  ms < it->init ) { --it; }
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

static double checkTime(lua_State* L, const int where, const bool local = false) noexcept {
	if( double S, T;  lua_istable(L, where) )
		return (local ? S : N.sinceTone) = barToTone(( lua_rawgeti(L, where, 1), lua_tonumber(L, -1)
			   )),																			lua_pop(L, 1),
			   T = ( lua_rawgeti(L, where, 2), lua_tonumber(L, -1) ),						lua_pop(L, 1),
			   barToBeat(toneToBar(  (local ? S : N.sinceTone) + ( T<0 ? -T/16 : T )  ));
	else
		return T = lua_tonumber(L, where),
			   barToBeat(toneToBar(  N.sinceTone + ( T<0 ? -T/16 : T )  ));
}   // Stack balanced; Won't pop

static N4::Point checkPointArg(lua_State* L, const int where) noexcept {   // Stack balanced; Won't pop
	if( lua_istable(L, where) ) {
		const auto r = ( lua_rawgeti(L, where, 1), lua_tonumber(L, -1) );					lua_pop(L, 1);
		const auto d = ( lua_rawgeti(L, where, 2), lua_tonumber(L, -1) );					lua_pop(L, 1);
		const auto e = ( lua_rawgeti(L, where, 3), lua_tointeger(L, -1) );					lua_pop(L, 1);
		return {
			.radius = fmin((uint8_t)(r * 4), 31) * 0.25,
			.degree = d < -1024 ? -1024 : d > 1023 ? 1023 : d,
			.ease = (uint8_t)(e > Arf4::OUTSINE  ?  Arf4::LINEAR : e)
		};
	}
	const auto e = lua_tonumber(L, where);
	return {.ease = (uint8_t)(e > Arf4::OUTSINE  ?  Arf4::LINEAR : e)};
}

static int wishGetInfo(lua_State* L) noexcept {
	const auto w = (N4::Wish*)lua_touserdata(L, 1);
	wishCacheT( *w, checkTime(L, 2, true) );
	lua_createtable(L, 0, 6);

	return
		lua_pushnumber(L, w->wX),		lua_setfield(L, -2, "x"),
		lua_pushnumber(L, w->wY),		lua_setfield(L, -2, "y"),
		lua_pushnumber(L, w->wNx),		lua_setfield(L, -2, "node_x"),
		lua_pushnumber(L, w->wNy),		lua_setfield(L, -2, "node_y"),
		lua_pushnumber(L, w->wRadius),	lua_setfield(L, -2, "radius"),
		lua_pushnumber(L, w->wDegree),	lua_setfield(L, -2, "degree"),
	1;
}

static int freeHelper(lua_State* L) noexcept {
	( (N4::Wish*)lua_touserdata(L, 1) ) -> ~Wish();
	return 0;
}


/* Error Reasons */
constexpr auto
	NEMESIS_TIME_OOR = R"(Nemesis Compiler: Time(ms) of %s out of range)",
	NEMESIS_SLE = R"(Nemesis Compiler: Count limit of %s(%L) exceeded)",
	NOT_A_TABLE = R"(API "%s" requires a Lua Table for the Arg 1.)",
	TIME_OUT_OF_RANGE = R"(API "%s": Time out of range in Args)",
	NO_WISH = R"(API "%s": No valid Wish to add "%s"(s) to)";
#define LI (lua_Integer)


/* Script APIs */
static std::map<double, N4::Delta> bpmMap;
static std::map<double, N4::Tempo> tempoMap;
int Ar::NewBuild(lua_State* L) noexcept {
	/* Example:
	 * Time {						-- For 4/4-only tracks
	 *     Offset = 0,				-- Beat 0 starts from 0ms
	 *     0, 170,					-- Bar, BPM
	 *     ···
	 * }
	 * Time {						-- For tracks with Tempo Variations
	 *     Offset = 0,				-- Offset must be positive
	 *     Tempo = {
	 *         0, 4, 4,				-- Bar, Beat Count of a Bar, Tone Divisor
	 *         1, 3, 4,
	 *         25, 4, 4
	 *     },
	 *     0, 0, 201,				-- Bar(to be converted to Beat), Additional Beats, BPM
	 *     ···
	 * }
	 */
	if(! lua_istable(L, 1) )
		return lua_pushboolean(L, false), lua_pushfstring(L, NOT_A_TABLE, "Time"), 2;
	const double offset = (lua_getfield(L, 1, "Offset"), fmax( lua_tonumber(L,-1), 0 ));
		lua_pop(L, 1);
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
	if( const size_t bpmInputLen = lua_objlen(L, 1);  tempoMap.empty() )
		for( size_t i = 1;  i < bpmInputLen;  i += 2 ) {
			const double beat = barToBeat(( lua_rawgeti(L, 1, i), lua_tonumber(L, -1) )),   // >=0 Clamped
						  bpm = ( lua_rawgeti(L, 1, i+1), lua_tonumber(L, -1) );
			bpmMap[beat] = { .init = beat,  .value = 60000 / (bpm<0 ? 170.0 : bpm) };
			lua_pop(L, 2);
		}
	else {
		const size_t tempoCount = tempoMap.size();
		N.tempoList.reserve(tempoCount);
		N.tempoList.clear();

		for( const auto& [_, tempo] : tempoMap )
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
			const double bpm  = ( lua_rawgeti(L, 1, i+2), lua_tonumber(L, -1) );
				  double beat = barToBeat(( lua_rawgeti(L, 1, i), lua_tonumber(L, -1) ))
							  + ( lua_rawgeti(L, 1, i+1), lua_tonumber(L, -1) );
						 beat = beat < 0  ?  0 : beat;
			bpmMap[beat] = { .init = beat,  .value = 60000 / (bpm<0 ? 170.0 : bpm) };
			lua_pop(L, 3);
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
			for( const auto& [_, node] : bpmMap )
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
		lua_pushcfunction(L, wishGetInfo),		lua_setfield(L, -2, "__call");
	if( luaL_newmetatable(L, "NEMESIS_HELPER") )
		lua_pushcfunction(L, wishGetInfo),		lua_setfield(L, -2, "__call"),
		lua_pushcfunction(L, freeHelper),		lua_setfield(L, -2, "__gc");
	return lua_pushboolean(L, true), 1;
}

static std::map<double, double> deltaMap;
int Ar::SetDelta(lua_State* L) noexcept {
	/* Example:
	 * Delta {
	 *     {0},			1,			-- Bar 0, Ratio: 1
	 *     {2, 1/32},	-1,			-- Bar 2, then 1/32 Tone, Ratio: -1
	 *     {2, 1},		0.9,		-- Bar 2, then 1/16 Tone, Ratio: 0.9
	 *     15,			1,			-- Bar 2(Cached), then 15/16 Tone, Ratio: 1
	 *     ···
	 * }
	 */
	if(! lua_istable(L, 1) )
		return lua_pushboolean(L, false), lua_pushfstring(L, NOT_A_TABLE, "Delta"), 2;
	deltaMap.clear();

	const size_t inputLen = lua_objlen(L, 1);
	for( size_t i = 1;  i < inputLen;  i += 2 ) {
		deltaMap[ beatToMs(( lua_rawgeti(L, 1, i), checkTime(L, -1) )) ]
			  = ( lua_rawgeti(L, 1, i+1), lua_tonumber(L, -1) );
		lua_pop(L, 2);
	}

	switch( auto SZ = deltaMap.size();  SZ ) {
		case 0:
			N.deltas[0] = { 0, 1 };
			return lua_pushboolean(L, true), 1;
		case 1:
			N.deltas[0] = { 0, fmax(deltaMap.cbegin()->second, 0) };
			return lua_pushboolean(L, true), 1;
		default:
			N.deltas.clear();
			N.deltas.reserve( deltaMap.size() );
			for( const auto [ms, ratio] : deltaMap )
				if( N.deltas.empty()  ||  ratio != N.deltas.back().value )
					N.deltas.push_back({ (uint64_t)(ms/4) * 4.0, ratio });

			if( N.deltas.front().value < 0 )
				N.deltas.front().value = 1;
			if( N.deltas.back().value < 0 )
				N.deltas.back().value = 1;
			N.deltas[0].init = 0;

			SZ = N.deltas.size();
			for( size_t i = 1;  i < SZ;  ++i ) {
				const auto  lastNode = N.deltas[i-1];
					  auto& thisNode = N.deltas[i];
				thisNode.base = lastNode.base + (thisNode.init - lastNode.init) * lastNode.value;

				if( thisNode.base < 0  ||  thisNode.base > 1048575 * (8 - 1.0/1024) )
					return N.deltas.clear(),  N.deltas.push_back({ 0, 1 }),
						   lua_pushboolean(L, false),  lua_pushfstring(L, TIME_OUT_OF_RANGE, "Delta"), 2;
			}
			return lua_pushboolean(L, true), 1;
	}
}

static std::map<double, N4::Point> nodeMap;
int Ar::NewWish(lua_State* L) noexcept {
	/* Example:
	 * local myWish = Wish {		-- When failed, a nil will be returned.
	 *     Special = true,			-- false by default
	 *     WithDelta = true,		-- true by default
	 *     {1}, 4, 3, LINEAR,		-- Bar 1, X=4, Y=3, Linear Ease
	 *
	 *     -- Add Radius(5 here) & Degree(0 here) like this
	 *     -12, oldWish {1,-12}.x, oldWish {1,-12}.y, {5, 0, LINEAR},
	 *     ···
	 * }
	 */
	if(! lua_istable(L, 1) )
		return lua_pushnil(L), lua_pushfstring(L, NOT_A_TABLE, "Wish"), 2;
	N4::Wish W = {
		.isSpecial = (uint8_t)( lua_getfield(L, 1, "Special"), lua_toboolean(L,-1) ),
		.withDt = (uint8_t)( lua_getfield(L, 1, "WithDelta"), lua_isnil(L,-1) ? true : lua_toboolean(L,-1) )
	};
	lua_pop(L, 2);

	nodeMap.clear();
	const size_t inputLen = lua_objlen(L, 1);
	for( size_t i = 1;  i < inputLen;  i += 4 ) {
		const double beat = ( lua_rawgeti(L, 1, i), checkTime(L, -1) ),
					 x = ( lua_rawgeti(L, 1, i+1), lua_tonumber(L, -1) ),
					 y = ( lua_rawgeti(L, 1, i+2), lua_tonumber(L, -1) );
		auto point = ( lua_rawgeti(L, 1, i+3), checkPointArg(L, -1) );
			 point.x = x < -23.875 ? -23.875  :  x > 39.875 ? 39.875  :  x;
			 point.y = y < -11.875 ? -11.875  :  y > 19.875 ? 19.875  :  y;
			 point.beat = beat;
		nodeMap[( nodeMap.contains(beat) ? nextDouble(beat) : beat )] = point;
		lua_pop(L, 4);
	}
	if( nodeMap.empty()  ||  nodeMap.crbegin()->second.beat <= nextDouble( nodeMap.cbegin()->second.beat ) )
		return lua_pushnil(L), 1;

	W.nodes.reserve( nodeMap.size() );
	for( const auto& [_, node] : nodeMap )
		W.nodes.push_back(node);
	W.nodes.back().ease = STATIC;

	lua_pushlightuserdata(L, ( N.wishes.push_back(W), &N.wishes.back() ));
	luaL_getmetatable(L, "NEMESIS_WISH");
	lua_setmetatable(L, -2);
	return 1;
}

int Ar::NewHelper(lua_State* L) noexcept {
	/* Example:
	 * local helper = Helper {		-- When failed, a nil will be returned.
	 *     {1}, 4, 3, LINEAR,		-- Bar 1, X=4, Y=3, Linear Ease
	 *     -12, 8, 9, LINEAR,		-- Bar 1 then 12/16 Tone, X=8, Y=9, Linear Ease
	 *     ···
	 * }   -- Then you can use the Helper to do some interpolations.
	 */
	if(! lua_istable(L, 1) )
		return lua_pushnil(L), lua_pushfstring(L, NOT_A_TABLE, "Helper"), 2;
	const auto  W = new(lua_newuserdata( L, sizeof(N4::Wish) )) N4::Wish;
		  auto& nodes = W -> nodes;
	nodeMap.clear();

	const size_t inputLen = lua_objlen(L, 1);
	for( size_t i = 1;  i < inputLen;  i += 4 ) {
		const double beat = ( lua_rawgeti(L, 1, i), checkTime(L, -1) ),
					 x = ( lua_rawgeti(L, 1, i+1), lua_tonumber(L, -1) ),
					 y = ( lua_rawgeti(L, 1, i+2), lua_tonumber(L, -1) );
		auto point = ( lua_rawgeti(L, 1, i+3), checkPointArg(L, -1) );
			 point.x = x < -23.875 ? -23.875  :  x > 39.875 ? 39.875  :  x;
			 point.y = y < -11.875 ? -11.875  :  y > 19.875 ? 19.875  :  y;
			 point.beat = beat;
		nodeMap[( nodeMap.contains(beat) ? nextDouble(beat) : beat )] = point;
		lua_pop(L, 4);
	}
	if( nodeMap.empty() )
		return W->~Wish(), lua_pushnil(L), 1;   // Let Lua GC free the mem of W

	nodes.reserve( nodeMap.size() );
	for( const auto& [_, node] : nodeMap )
		nodes.push_back(node);
	nodes.back().ease = STATIC;

	luaL_getmetatable(L, "NEMESIS_HELPER");
	lua_istable(L, -1) ? (void)lua_setmetatable(L, -2) : W->~Wish();
	return 1;
}


int Ar::NewChild(lua_State* L) noexcept {
	/* Example:
	 * Child {
	 *     Wish = nil,				-- The last Wish of the Fumen by default
	 *     Radius = 7.0,			-- 7.0 by Default
	 *     Special = false,			-- Try to generate a special Hint if true, false by default
	 *     InitLoop = 0.25,			-- 0.25 by default
	 *     DeltaLoop = 1.25,		-- 0 by default
	 *     {1, 1}, 2, 3, 4, ···		-- Times
	 * }
	 */
	if( N.wishes.empty() )
		return lua_pushboolean(L, false), lua_pushfstring(L, NO_WISH, "Child", "WishChild"), 2;
	if(! lua_istable(L, 1) )
		return lua_pushboolean(L, false), lua_pushfstring(L, NOT_A_TABLE, "Child"), 2;

	auto W = &N.wishes.back();
	if( lua_getfield(L, 1, "Wish"), lua_getmetatable(L, -1), luaL_getmetatable(L, "NEMESIS_WISH"),
		lua_equal(L, -1, -2) )
		W = (N4::Wish*)lua_touserdata(L, 2);
	double radius = ( lua_getfield(L, 1, "Radius"), lua_tonumber(L, -1) );
		   radius = radius ? (radius > 7.75 ? 7.75 : radius) : 7;
	double initLoop = ( lua_getfield(L, 1, "InitLoop"),  lua_isnil(L,-1) ? 0.25 : lua_tonumber(L,-1) );
		   initLoop = initLoop < 0 ? 0  :  initLoop > 1 ? 1  :  initLoop;
	double deltaLoop = ( lua_getfield(L, 1, "DeltaLoop"), lua_tonumber(L, -1) );
		   deltaLoop = deltaLoop < -1.875 ? -1.875  :  deltaLoop > 1.875 ? 1.875  :  deltaLoop;
	const bool hintSpecial = ( lua_getfield(L, 1, "Special"), lua_toboolean(L, -1) );
	lua_pop(L, 4);

	const double minBeat = W->nodes.front().beat;
	const size_t inputLen = lua_objlen(L, 1);			W->wishChilds.reserve( inputLen );
	for( size_t i = 1;  i <= inputLen;  ++i )
		if( const double beat = (lua_rawgeti(L, 1, i), checkTime(L, -1));  lua_pop(L, 1),  beat > minBeat )
			W->wishChilds.push_back({ beat, radius, initLoop, deltaLoop, hintSpecial });
	return lua_pushboolean(L, true), 1;
}

int Ar::NewHint(lua_State* L) noexcept {
	/* Usage:
	 * Hint {
	 *     Wish = myWish,			-- The last Wish of the Fumen by default
	 *     Special = false,			-- False by default
	 *     {1}, 1, 2, 3, 4, ···		-- Times
	 * }
	 */
	if( N.wishes.empty() )
		return lua_pushboolean(L, false), lua_pushfstring(L, NO_WISH, "Hint", "Hint"), 2;
	if(! lua_istable(L, 1) )
		return lua_pushboolean(L, false), lua_pushfstring(L, NOT_A_TABLE, "Hint"), 2;

	auto W = &N.wishes.back();
	if( lua_getfield(L, 1, "Wish"), lua_getmetatable(L, -1), luaL_getmetatable(L, "NEMESIS_WISH"),
		lua_equal(L, -1, -2) )
		W = (N4::Wish*)lua_touserdata(L, 2);
	const bool hintSpecial = ( lua_getfield(L, 1, "Special"), lua_toboolean(L, -1) );
	lua_pop(L, 1);

	const double minBeat = W->nodes.front().beat,
				 maxBeat = W->nodes.back().beat;
	const size_t inputLen = lua_objlen(L, 1);			W->manualHints.reserve( inputLen );
	for( size_t i = 1;  i <= inputLen;  ++i )
		if( const double beat = ( lua_rawgeti(L, 1, i), checkTime(L, -1) );  lua_pop(L, 1),
			beat > minBeat  &&  beat <= maxBeat )
			W->manualHints.push_back({ beat, hintSpecial });
	return lua_pushboolean(L, true), 1;
}

int Ar::NewEcho(lua_State* L) noexcept {
	/* Usage:
	 * Echo {
	 *     Radius = 7.0,			-- 0 by Default
	 *     Special = false,			-- Scored if true, false by default
	 *     InitLoop = 0.25,			-- 0.25 by default, ignored if Radius is 0
	 *     DeltaLoop = 1.25,		-- 0 by default, ignored if Radius is 0
	 *     {1}, 8, 0.5,				-- T1, X1, Y1
	 *     12, 8, 0.5,				-- T2, X2, Y2
	 *     ···
	 * }
	 */
	if(! lua_istable(L, 1) )
		return lua_pushboolean(L, false), lua_pushfstring(L, NOT_A_TABLE, "Echo"), 2;
	const bool isSpecial = (lua_getfield(L, 1, "Special"), lua_toboolean(L, -1));
		   double radius = (lua_getfield(L, 1, "Radius"), lua_tonumber(L, -1)), initLoop = 0, deltaLoop = 0;
				  radius = radius > 7.75 ? 7.75  :  radius < 0 ? 0  :  radius;
	lua_pop(L, 2);

	if( radius )
		initLoop = ( lua_getfield(L, 1, "InitLoop"), lua_tonumber(L, -1) ),
		initLoop = initLoop ? (initLoop < 0 ? 0  :  initLoop > 1 ? 1  :  initLoop) : 0.25,
		deltaLoop = ( lua_getfield(L, 1, "DeltaLoop"), lua_tonumber(L, -1) ),
		deltaLoop = deltaLoop < -1.875 ? -1.875  :  deltaLoop > 1.875 ? 1.875  :  deltaLoop,
	/**/lua_pop(L, 2);

	const size_t inputLen = lua_objlen(L, 1);			N.echoes.reserve(inputLen);
	for( size_t i = 1;  i < inputLen;  i += 3 ) {
		const double beat = ( lua_rawgeti(L, 1, i), checkTime(L, -1) ),
					 x = ( lua_rawgeti(L, 1, i+1), lua_tonumber(L, -1) ),
					 y = ( lua_rawgeti(L, 1, i+2), lua_tonumber(L, -1) );
		N.echoes.push_back({
			.x = x < -23.875 ? -23.875  :  x > 39.875 ? 39.875  :  x,
			.y = y < -11.875 ? -11.875  :  y > 19.875 ? 19.875  :  y,
			beat, radius, initLoop, deltaLoop, isSpecial
		});
	}
	return lua_pushboolean(L, true), 1;
}

int Ar::NewVerse(lua_State* L) noexcept {
	/* Usage:
	 * Verse(since_tone) do ... end
	 */
	if( !lua_isnoneornil(L, 1) )
		(void)checkTime(L, 1);
	return N.verseWidx = N.wishes.size(),
		   N.verseEidx = N.echoes.size(),
	0;
}

int Ar::Mirror(lua_State* L) noexcept {
	/* Usage:
	 * Mirror(mirror_lr, mirror_ud)
	 */
	switch( lua_toboolean(L, 1) + (lua_toboolean(L, 2) << 1) ) {
		case 1:   // Mirror LR
			for( auto& w : std::span(N.wishes).subspan(N.verseWidx) ) {
				for( auto& node : w.nodes )
					node.x = 16 - node.x;
				for( auto& child : w.wishChilds )
					child.initLoop = (child.initLoop > 0.5 ? 1.5 : 0.5) - child.initLoop,
					child.deltaLoop = -child.deltaLoop;
			}
			for( auto& e : std::span(N.echoes).subspan(N.verseEidx) )
				e.x = 16 - e.x,
				e.initLoop = (e.initLoop > 0.5 ? 1.5 : 0.5) - e.initLoop,
				e.deltaLoop = -e.deltaLoop;
			break;
		case 2:   // Mirror UD
			for( auto& w : std::span(N.wishes).subspan(N.verseWidx) ) {
				for( auto& node : w.nodes )
					node.y = 8 - node.y;
				for( auto& child : w.wishChilds )
					child.initLoop = 1 - child.initLoop,
					child.deltaLoop = -child.deltaLoop;
			}
			for( auto& e : std::span(N.echoes).subspan(N.verseEidx) )
				e.y = 8 - e.y,
				e.initLoop = 1 - e.initLoop,
				e.deltaLoop = -e.deltaLoop;
			break;
		case 3:   // Mirror LR & UD
			for( auto& w : std::span(N.wishes).subspan(N.verseWidx) ) {
				for( auto& node : w.nodes )
					node.x = 16 - node.x,  node.y = 8 - node.y;
				for( auto& child : w.wishChilds )
					child.initLoop = child.initLoop + (child.initLoop > 0.5 ? -0.5 : 0.5);
			}
			for( auto& e : std::span(N.echoes).subspan(N.verseEidx) )
				e.x = 16 - e.x,  e.y = 8 - e.y,
				e.initLoop = e.initLoop + (e.initLoop > 0.5 ? -0.5 : 0.5);
		default:;
	}
	return 0;
}

int Ar::DeltaTone(lua_State* L) noexcept {
	/* Example:
	 * local delta_tone = DeltaTone( {1, 1/16}, {2, 5/32} )
	 */
	double LT;
	if( lua_istable(L, 1) )
		LT = ( lua_rawgeti(L, 1, 2), lua_tonumber(L, -1) ),										// withTone
		LT = barToTone(( lua_rawgeti(L, 1, 1), lua_tonumber(L, -1) )) + ( LT<0 ? -LT/16 : LT ); // sinceBar
	else
		LT = lua_tonumber(L, 1),  LT = N.sinceTone + ( LT<0 ? -LT/16 : LT );

	double RT;
	if( lua_istable(L, 2) )
		RT = ( lua_rawgeti(L, 2, 2), lua_tonumber(L, -1) ),										// withTone
		RT = barToTone(( lua_rawgeti(L, 2, 1), lua_tonumber(L, -1) )) + ( RT<0 ? -RT/16 : RT ); // sinceBar
	else
		RT = lua_tonumber(L, 2),  RT = N.sinceTone + ( RT<0 ? -RT/16 : RT );

	lua_pushnumber( L, RT - LT );
	return 1;
}

int Ar::BarToMs(lua_State* L) noexcept {
	/* Usage:
	 * local ms = BarToMs(absolute_bar_value)
	 */
	return lua_pushnumber(L, beatToMs(barToBeat( lua_tonumber(L,1) ))), 1;
}


/* Arf Compile Fn */
static std::map<uint64_t, int16_t> valueMap;
static std::vector< std::vector<Arf4::Wish> > idxProto;
int Ar::OrganizeArf(lua_State* L) noexcept {
	/* Usage:
	 * local before_or_false, objcnt, wgo_required, hgo_required, ego_required = Arf4.OrganizeArf()
	 */
	Fumen F = { .val = 0 };

	// Organize Deltas
	F.deltas.reserve( N.deltas.size() + 1 ), F.deltas.push_back({ .val = 0 });
	for( const auto [init, value, base] : N.deltas ) {
		if( init > 1048575 )   /* Arf4.h */
			return lua_pushboolean(L, false), lua_pushfstring(L, NEMESIS_TIME_OOR, "DeltaNode"), 2;
		F.deltas.push_back({ .t = (uint64_t)init >> 2,
							 .absV = (uint64_t)( fmin( abs(value), 8 - 1.0/1024 ) * 1024 ),
							 .base = (uint64_t)( base * 1024 ) });
	}
	if( F.deltas.size() > 8191 )   /* inout.cpp 1/9 */
		return lua_pushboolean(L, false), lua_pushfstring(L, NEMESIS_SLE, "DeltaNodes", LI 8190), 2;

	// Organize Echoes
	valueMap.clear();
	for( const auto [x, y, beat, radius, initLoop, deltaLoop, isSpecial] : N.echoes )
		if( const uint64_t ms = beatToMs(beat);  ms < 637  ||  ms > 1048575 - 470 )   /* Arf4.h */
			return lua_pushboolean(L, false), lua_pushfstring(L, NEMESIS_TIME_OOR, "Echo"), 2;
		else if( const Echo baseEcho = { .cdx = (int64_t)( (x - 8) * 8 ),			.ms = ms,
										 .cdy = (int64_t)( (y - 4) * 8 ),			.status = 0,
										 .radius = (uint64_t)( radius * 4 ),		.deltaMs = 0,
										 .initLoop = (uint64_t)( initLoop * 64 ),
										 .deltaLoop = (int64_t)( deltaLoop * 8 ) };
		valueMap[baseEcho.val] == false )   // Insertion and Value Checking, in one sentence
			valueMap[baseEcho.val] = isSpecial;

	const size_t echoSize = valueMap.size();
	if( echoSize > 32767 )   /* inout.cpp 2/9 */
		return lua_pushboolean(L, false), lua_pushfstring(L, NEMESIS_SLE, "Echoes", LI 32767), 2;
	F.echoes.reserve( echoSize );

	for( const auto [val, isSpecial] : valueMap )
		if( Echo e = { .val = val };  true )
			F.echoes.push_back(( e.status = isSpecial, e ));
	std::ranges::sort( F.echoes, [](const Echo a, const Echo b) { return  a.val << 27  <  b.val << 27; } );

	// Organize Times of Wishes, Add Hints into `F.hints`
	valueMap.clear();
	for( auto& w : N.wishes ) {
		for( auto& n : w.nodes )
			if( (n.beat = beatToMs( n.beat )) > 1048575 )   /* Arf4.h */
				return lua_pushboolean(L, false), lua_pushfstring(L, NEMESIS_TIME_OOR, "Wish Node"), 2;
		if( w.nodes.size() > 63 )   /* Arf4.h */
			return lua_pushboolean(L, false), lua_pushfstring(L, NEMESIS_SLE, "Nodes of a Wish", LI 63), 2;

		for( auto& c : w.wishChilds ) {   // Use valueMap to deduplicate & sort childs later
			if( (c.beat = beatToMs( c.beat )) <= w.nodes.back().beat  &&  c.beat >= 510 ) {
				wishCacheT(w, c.beat);
				if( const Hint baseHint = { .cdx = (int64_t)( (w.wX - 8) * 8 ),		.status = 0,
											.cdy = (int64_t)( (w.wY - 4) * 8 ),		.deltaMs = 0,
											.ms = (uint64_t)c.beat };
				valueMap[baseHint.val] == false )
					valueMap[baseHint.val] = c.hintSpecial;
			}
		}
		if( w.withDt )  for( auto& c : w.wishChilds )
			c.beat = msToDt(c.beat);

		for( auto [beat, isSpecial] : w.manualHints )
			if( beat = beatToMs(beat), beat >= 510 ) {
				wishCacheT(w, beat);
				if( const Hint baseHint = { .cdx = (int64_t)( (w.wX - 8) * 8 ),		.status = 0,
											.cdy = (int64_t)( (w.wY - 4) * 8 ),		.deltaMs = 0,
											.ms = (uint64_t)beat };
				valueMap[baseHint.val] == false )
					valueMap[baseHint.val] = isSpecial;
			}
	}

	const size_t hintSize = valueMap.size();
	if( hintSize > 32767 )   /* inout.cpp 3/9 */
		return lua_pushboolean(L, false), lua_pushfstring(L, NEMESIS_SLE, "Hints", LI 32767), 2;
	F.hints.reserve( hintSize );

	for( const auto [val, isSpecial] : valueMap )
		if( Hint h = { .val = val };  true )
			F.hints.push_back(( h.status = isSpecial, h ));

	/* Generate hIdx & eIdx, Count scored objects
	 * Metadata: before, objectCount, hgoRequired, egoRequired
	 */
	const int16_t hIdxSize = (   /* inout.cpp "hIdx" 4/9 */
		F.hints.empty()  ?  -1 : (F.before = F.hints.back().ms + 470) >> 10   // 1st time to assign F.before
	) + 1;
	idxProto.clear(), idxProto.resize( hIdxSize );

	for( size_t i = 0;  i < hintSize;  ++i ) {
		const Hint h = F.hints[i];
		if( F.sHit < 31 )		F.sHit += h.status;
		else					F.hints[i].status = NJUDGED;

		const size_t endGroup = (h.ms + 470) >> 10;
		for( size_t group = (h.ms - 510) >> 10;  group <= endGroup;  ++group )
			idxProto[group].push_back({ .val = i });
	}

	F.hIdx.reserve( hIdxSize );
	for( const auto& group : idxProto )
		if( uint32_t since, count;  group.empty() )
			F.hIdx.push_back({ .f = 0, .c = 0 });
		else if( since = group[0].val,  count = group.back().val - since + 1,  count > 511 )   /* Arf4.h */
			return lua_pushboolean(L, 0), lua_pushfstring(L, NEMESIS_SLE, "Hints within 1024ms", LI 511), 2;
		else if( F.hIdx.push_back({ .f = since, .c = count }),  count > F.hgoRequired )
			F.hgoRequired = count;

	const int16_t eIdxSize = (   /* inout.cpp "eIdx" 5/9 */
		F.echoes.empty()  ?  -1 : (F.before = fmax( F.before, F.echoes.back().ms + 470 )) >> 10
	) + 1;
	idxProto.clear(), idxProto.resize( eIdxSize );

	F.objectCount = hintSize;
	for( size_t i = 0;  i < echoSize;  ++i ) {
		const Echo e = F.echoes[i];
		F.objectCount += e.status;

		int32_t initMs = e.ms - (e.radius ? 1011 : 637);
		if( initMs < 0 )
			initMs = 0;

		const size_t endGroup = (e.ms + 470) >> 10;
		for( size_t group = initMs >> 10;  group <= endGroup;  ++group )
			idxProto[group].push_back({ .val = i });
	}

	F.eIdx.reserve( eIdxSize );
	for( const auto& group : idxProto )
		if( uint32_t since, count;  group.empty() )
			F.eIdx.push_back({ .f = 0, .c = 0 });
		else if( since = group[0].val,  count = group.back().val - since + 1,  count > 511 )   /* Arf4.h */
			return lua_pushboolean(L,0), lua_pushfstring(L, NEMESIS_SLE, "Echoes within 1024ms", LI 511), 2;
		else if( F.eIdx.push_back({ .f = since, .c = count }),  count > F.egoRequired )
			F.egoRequired = count;

	/* Flatten Nodes & WishChilds
	 * Metadata: before
	 */
	idxProto.clear(), idxProto.resize(511);
	std::ranges::sort( N.wishes, [](const auto& a, const auto& b) {
		return a.nodes.front().beat < b.nodes.front().beat;
	});

	F.nodes.reserve(32767);
	F.wishChilds.reserve(32767);
	for( auto& w : N.wishes ) {
		Wish wView = { .nSince = F.nodes.size(), .nCount = w.nodes.size(),	.cSince = 0, .cCount = 0,
					   .withDt = w.withDt, .isSpecial = w.isSpecial,		.nIndex = 0, .cIndex = 1 };
		// Organize Nodes
		// Time & Count Checked
		for( const auto [x, y, beat, radius, degree, ease] : w.nodes )
			F.nodes.push_back({ .cdx = (int64_t)( (x - 8) * 8 ),  .ms = (uint64_t)beat,
								.cdy = (int64_t)( (y - 4) * 8 ),  .ease = ease,
								.radius = (uint64_t)( radius * 4 ),
								.deg = (int64_t)degree });
		// Organize WishChilds
		valueMap.clear();
		for(const auto& nC : w.wishChilds) {
			const Child c = { .radius = (uint64_t)( nC.radius * 4 ),
							  .initLoop = (uint64_t)( nC.initLoop * 64 ),
							  .deltaLoop = (int64_t)( nC.deltaLoop * 8 ),
							  .zDt = (uint64_t)( nC.beat * 1024 ) };
			valueMap[c.val] = 0;
		}

		if( valueMap.size() > 1023 )   /* Arf4.h */
			return lua_pushboolean(L,0), lua_pushfstring(L, NEMESIS_SLE, "a Wish's WishChilds", LI 1023), 2;
		if( valueMap.size() ) {
			wView.cSince = F.wishChilds.size();
			for( const auto [cVal, _] : valueMap )
				F.wishChilds.push_back({ .val = cVal });
			wView.cCount = valueMap.size();
		}

		// Size Check (inout.cpp)
		if( F.nodes.size() > 32767 )   /* 6/9 */
			return lua_pushboolean(L, false), lua_pushfstring(L, NEMESIS_SLE, "Wish Nodes", LI 32767), 2;
		if( F.wishChilds.size() > 32767 )   /* 7/9 */
			return lua_pushboolean(L, false), lua_pushfstring(L, NEMESIS_SLE, "WishChilds", LI 32767), 2;

		// Calculate wgoRequired for this Wish (Here the `.cIndex` field is borrowed)
		valueMap.clear();
		for( const auto& c : w.wishChilds ) {
			constexpr double LOWEST_SPEED = 11 / 1500.0;
				const double to = c.beat * LOWEST_SPEED,
							 from = to - fmax(c.radius, 6) * LOWEST_SPEED;
			valueMap[from] += 1, valueMap[to] -= 1;
		}

		int16_t currentStep = 1;   // The Wish itself
		for( const auto [_, stepDelta] : valueMap )
			if( (currentStep += stepDelta) > wView.cIndex )
				wView.cIndex = currentStep;

		/* Push this Wish into wIdxProto
		 * Update F.before
		 */
		const uint32_t lastMs = w.nodes.back().beat;
		if( F.before < lastMs )
			F.before = lastMs;

		const uint16_t endGroup = lastMs >> 11;
		for( uint32_t i = (uint32_t)w.nodes.front().beat >> 11;  i <= endGroup;  ++i )
			idxProto[i].push_back( wView );
	}

	const auto wIdxSize = (F.before >> 11) + 1, oIdxSize = (F.before >> 10) + 1;
	F.wIdx.reserve( wIdxSize ),  F.hIdx.resize( oIdxSize ),  F.eIdx.resize( oIdxSize );
	idxProto.resize(wIdxSize);   /* inout.cpp "wIdx" 8/9 */

	/* Flatten Wishes
	 * Metadata: wgoRequired
	 */
	F.wishes.reserve(65535);
	for( auto& group : idxProto )
		if( const uint16_t groupSize = group.size();  !groupSize ) [[unlikely]]
			F.wIdx.push_back({ .f = 0, .c = 0 });
		else if( uint16_t groupWgoUsed = 0;  groupSize > 1023 )  [[unlikely]]   /* Arf4.h */
			return lua_pushboolean(L, false),
				   lua_pushfstring(L, NEMESIS_SLE, "Wishes within 2048ms", LI 1023), 2;
		else [[likely]] {
			F.wIdx.push_back({ .f = (uint32_t)F.wishes.size(),  .c = (uint32_t)groupSize });
			for( auto wish : group )
				groupWgoUsed += wish.cIndex,	wish.cIndex = 0 /* End of the borrow */,
				F.wishes.push_back( wish );
			if( F.wishes.size() > 65535 )   /* inout.cpp 9/9 */
				return lua_pushboolean(L, false), lua_pushfstring(L, NEMESIS_SLE, "Wishes", LI 65535), 2;
			if( groupWgoUsed > 1023 )   /* Arf4.h */
				return lua_pushboolean(L, false),
					   lua_pushfstring(L, NEMESIS_SLE, "Wishes within 2048ms", LI 1023), 2;
			F.wgoRequired = groupWgoUsed > F.wgoRequired  ?  groupWgoUsed : F.wgoRequired;
		}

	return  Arf = { .val = F.val, .isAuto = true },
			F.deltas.swap(Arf.deltas),				F.nodes.swap(Arf.nodes),
			F.echoes.swap(Arf.echoes),				F.wishes.swap(Arf.wishes),
			F.hints.swap(Arf.hints),				F.wishChilds.swap(Arf.wishChilds),
			F.hIdx.swap(Arf.hIdx),					F.wIdx.swap(Arf.wIdx),
			F.eIdx.swap(Arf.eIdx),					lua_pushinteger(L, F.before),
			lua_pushinteger(L, F.objectCount),		lua_pushinteger(L, F.wgoRequired),
			lua_pushinteger(L, F.hgoRequired),		lua_pushinteger(L, F.egoRequired),
	5;
}
#endif