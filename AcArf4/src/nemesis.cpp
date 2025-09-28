// Nemesis, the Aerials Fumen Compiler. //
#ifdef AR_BUILD_VIEWER
#include <algorithm>
#include <Arf4.h>
#include <span>
#include <map>

/* Strings & Error Reasons */
static constexpr auto
	N_WISH = "NEMESIS_WISH",  N_HELPER = "NEMESIS_HELPER", N_SPECIAL = "Special",
	RADIUS = "Radius", INITLOOP = "InitLoop", DELTALOOP = "DeltaLoop", DELTA = "Delta", LCALL = "__call",
	WISHES_2048 = "Wishes within 2048ms", WISH = "Wish", HINT = "Hint", ECHO = "Echo", CHILD = "Child";
static constexpr auto
	NOT_SUF = "Count of Node(s) is not sufficient to create a Wish / Helper.",
	NEMESIS_TIME_OOR = R"(Nemesis Compiler: Time(ms) of %s out of range)",
	NEMESIS_SLE = R"(Nemesis Compiler: Count limit of %s(%td) exceeded)",
	NOT_A_TABLE = R"(API "%s" requires a Lua Table for the Arg 1.)",
	TIME_OUT_OF_RANGE = R"(API "%s": Time out of range in Args)",
	NO_WISH = R"(API "%s": No valid Wish to add "%s"(s) to)";
#define LI (lua_Integer)

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
		std::vector<Hint>	hints;
		std::vector<Point>	nodes;
		std::vector<Child>	wishChilds;
		uint16_t			nIdx, isSpecial:8, withDt:8;
		//------------------------//
		float				wRadius;
		double				wX, wY, wNx, wNy, wDegree;
	};
	struct Build {
		std::vector<Tempo>	tempoList = {{ .a=4, .b=4 }};
		std::vector<Delta>	bpmList = {{ 0, 170 }},  deltas = {{ 0,1 }};
		std::vector<Wish>	wishes;
		std::vector<Echo>	echoes;
		uint64_t			verseWidx:24, verseEidx:17, dIdx:23;
		uint32_t			tIdx, bIdx;
		double				sinceTone;
	};
}

static N4::Build N;
static double barToTone(double bar) noexcept {   // With User Input
	if( const auto lastTempo = N.tempoList.back();  ( bar = fmax(bar, 0) ) >= lastTempo.bar )
		return lastTempo.toneBase + (bar - lastTempo.bar) * lastTempo.a / lastTempo.b;
	auto it = N.tempoList.begin() + N.tIdx;
		while( bar <  it[0].bar )  { --it; }
		while( bar >= it[1].bar )  { ++it; }
	return N.tIdx = it - N.tempoList.begin(),
		   it->toneBase + (bar - it->bar) * it->a / it->b;
}

static double toneToBeat(const double tone) noexcept {   // Internal, tone >= 0 required
	if( const auto lastTempo = N.tempoList.back();  tone >= lastTempo.toneBase )
		return lastTempo.beatBase + (tone - lastTempo.toneBase) * lastTempo.b;
	auto it = N.tempoList.begin() + N.tIdx;
		while( tone <  it[0].toneBase )  { --it; }
		while( tone >= it[1].toneBase )  { ++it; }
	return N.tIdx = it - N.tempoList.begin(),
		   it->beatBase + (tone - it->toneBase) * it->b;
}

static double beatToMs(const double beat) noexcept {   // Internal, beat >= 0 required
	if( const auto lastBpm = N.bpmList.back();  beat >= lastBpm.init )
		return lastBpm.base + (beat - lastBpm.init) * 60000 / lastBpm.value;
	auto it = N.bpmList.begin() + N.bIdx;
		while( beat <  it[0].init )  { --it; }
		while( beat >= it[1].init )  { ++it; }
		N.bIdx = it - N.bpmList.begin();
	if( double slope;  it[0].value < 0 ) [[unlikely]]   // `it+1 < N.bpmList.end()` Guaranteed
		return slope = ( fabs(it[1].value) + it[0].value ) / ( it[1].init - it[0].init ),
			   it->base + 60000 / slope * log( 1 + (beat - it->init) * slope / -it->value );
	return it->base + (beat - it->init) * 60000 / it->value;
}

static double msToDt(const double ms) noexcept {   // Internal, ms >= 0 required
	if( const auto lastDelta = N.deltas.back();  ms >= lastDelta.init )
		return lastDelta.base + (ms - lastDelta.init) * lastDelta.value;
	auto it = N.deltas.begin() + N.dIdx;
		while( ms <  it[0].init )  { --it; }
		while( ms >= it[1].init )  { ++it; }
	return N.dIdx = it - N.deltas.begin(),
		   it->base + (ms - it->init) * it->value;
}

static double nextDouble(const double d) noexcept {   // Little Endian Only
	union{ double d; uint64_t u; }  nd = {d};
	return ++nd.u, nd.d;
}

static void wishCacheT(N4::Wish& w, const double beat) noexcept {
	if( const auto& first = w.nodes.front();  beat < first.beat ) [[unlikely]]
		w.wNx = first.x, w.wNy = first.y, w.wRadius = first.radius, w.wDegree = first.degree;
	else if( const auto& last = w.nodes.back();  beat >= last.beat )
		w.wNx = last.x, w.wNy = last.y, w.wRadius = last.radius, w.wDegree = last.degree;
	else [[likely]] {
		auto it = w.nodes.cbegin() + w.nIdx;
			while( beat <  it[0].beat )  { --it; }
			while( beat >= it[1].beat )  { ++it; }
		const auto &thiz = it[0], &next = it[1];
		const auto ratio = Ar::Eased( (beat - thiz.beat) / (next.beat - thiz.beat),  thiz.ease );
			w.wNx = thiz.x * (1-ratio) + next.x * ratio;   // This makes less derefs
			w.wNy = thiz.y * (1-ratio) + next.y * ratio;
			w.wRadius = thiz.radius * (1-ratio) + next.radius * ratio;
			w.wDegree = thiz.degree * (1-ratio) + next.degree * ratio;
		w.nIdx = it - w.nodes.cbegin();
	}
	const auto cosSin = Ar::CosSin({ (float)w.wDegree });
	w.wX = w.wNx + w.wRadius * cosSin.a;
	w.wY = w.wNy + w.wRadius * cosSin.b;
}

static double checkTime(lua_State* L, const int where, const bool local = false) noexcept {
	if( double S, T;  lua_istable(L, where) )
		return (local ? S : N.sinceTone) = barToTone(( lua_rawgeti(L, where, 1), lua_tonumber(L, -1)
			   )),																			lua_pop(L, 1),
			   T = ( lua_rawgeti(L, where, 2), lua_tonumber(L, -1) ),						lua_pop(L, 1),
			   toneToBeat( (local ? S : N.sinceTone) + (T<0 ? -T/16 : T) );
	else [[likely]]
		return T = lua_tonumber(L, where),
			   toneToBeat( N.sinceTone + (T<0 ? -T/16 : T) );
}   // Stack balanced; Won't pop

static N4::Point checkPointArg(lua_State* L, const int where) noexcept {   // Stack balanced; Won't pop
	if( lua_istable(L, where) ) {
		const auto r = ( lua_rawgeti(L, where, 1), lua_tonumber(L, -1) );					lua_pop(L, 1);
		const auto d = ( lua_rawgeti(L, where, 2), lua_tonumber(L, -1) );					lua_pop(L, 1);
		const auto e = ( lua_rawgeti(L, where, 3), lua_tointeger(L, -1) );					lua_pop(L, 1);
		return { .radius = r < 0 ? 0  :  r > 15.75 ? 15.75  :  r,
				 .degree = d < -1024 ? -1024  :  d > 1023 ? 1023  :  d,
				 .ease = (uint8_t)(e > Arf4::OUTSINE ? Arf4::LINEAR : e) };
	}
	const auto e = lua_tonumber(L, where);
	return {.ease = (uint8_t)(e > Arf4::OUTSINE  ?  Arf4::LINEAR : e)};
}

static std::map<double, N4::Point> nodeMap;
static bool fillnEmpty(lua_State* L) noexcept {
	nodeMap.clear();
	for( size_t len = lua_objlen(L, 1),  i = 1;  i < len;  i += 4 ) {
		const double beat = ( lua_rawgeti(L, 1, i), checkTime(L, -1) ),
					 x = ( lua_rawgeti(L, 1, i+1), lua_tonumber(L, -1) ),
					 y = ( lua_rawgeti(L, 1, i+2), lua_tonumber(L, -1) );
		auto point = ( lua_rawgeti(L, 1, i+3), checkPointArg(L, -1) );
			 point.x = x < -23.9375 ? -23.9375  :  x > 39.9375 ? 39.9375  :  x;
			 point.y = y <   -3.875 ?   -3.875  :  y >  11.875 ?  11.875  :  y;
			 point.beat = beat;
		nodeMap[( nodeMap.contains(beat) ?
				( nodeMap[beat].ease = Ar::STATIC, nextDouble(beat) ) : beat )] = point;
		lua_pop(L, 4);
	}
	return nodeMap.empty();
}

static int wishGetInfo(lua_State* L) noexcept {
	const auto w = (N4::Wish*)lua_touserdata(L, 1);		// [1] Wish / Helper
	wishCacheT( *w, checkTime(L, 2, true) );			// [2] Time
	lua_createtable(L, 0, 6);							// [3]

	return
		lua_pushnumber(L, w->wX),		lua_setfield(L, 3, "x"),
		lua_pushnumber(L, w->wY),		lua_setfield(L, 3, "y"),
		lua_pushnumber(L, w->wNx),		lua_setfield(L, 3, "node_x"),
		lua_pushnumber(L, w->wNy),		lua_setfield(L, 3, "node_y"),
		lua_pushnumber(L, w->wRadius),	lua_setfield(L, 3, "radius"),
		lua_pushnumber(L, w->wDegree),	lua_setfield(L, 3, "degree"),
	1;
}

static int freeHelper(lua_State* L) noexcept {
	( (N4::Wish*)lua_touserdata(L, 1) ) -> ~Wish();
	return 0;
}


/* Script APIs */
static std::map<double, double> doubleMap;
static std::map<double, N4::Tempo> tempoMap;
int Ar::NewBuild(lua_State* L) noexcept {
	/* Examples:
	 * Time {						-- For 4/4-only tracks
	 *     Offset = 1,				-- Beat 0 starts from 1ms
	 *     0, 170,					-- Bar, BPM
	 *     ···
	 * }
	 * Time {						-- For tracks with Tempo Variations
	 *     Offset = 1,				-- Offset must be positive
	 *     Tempo = { 0, 4, 4,		-- Bar, Beat Count of a Bar, Tone Divisor
	 *				 1, 3, 4,
	 *				 25, 4, 4 },
	 *     0, 0, 201,				-- Bar(to be converted to Beat), Additional Beats, BPM
	 *     ···						-- Negative BPM as Linear BPM
	 * }
	 */
	if(! lua_istable(L, 1) )
		return lua_pushboolean(L, false), lua_pushfstring(L, NOT_A_TABLE, "Time"), 2;
	const double offset = (lua_getfield(L, 1, "Offset"), fmax( lua_tonumber(L,-1), 1 ));
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
	doubleMap.clear();
	if( const size_t bpmInputLen = lua_objlen(L, 1);  tempoMap.empty() ) [[likely]]
		for( size_t i = 1;  i < bpmInputLen;  i += 2 ) {
			const double beat = toneToBeat(barToTone(( lua_rawgeti(L, 1, i), lua_tonumber(L,-1) ))),
						  bpm = ( lua_rawgeti(L, 1, i+1), lua_tonumber(L, -1) );
			doubleMap[beat] = bpm ? bpm : 170.0;   // `beat > 0` clamped
			lua_pop(L, 2);
		}
	else {
		const size_t tempoCount = tempoMap.size();
		N.tempoList.clear(), N.tempoList.reserve(tempoCount);

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
				  double beat = toneToBeat(barToTone(( lua_rawgeti(L, 1, i), lua_tonumber(L,-1) )))
							  + ( lua_rawgeti(L, 1, i+1), lua_tonumber(L, -1) );
			doubleMap[ (beat < 0) ? 0 : beat ] = bpm ? bpm : 170.0;
			lua_pop(L, 3);
		}
	}

	// Organize BPMs
	switch( const size_t bpmCnt = doubleMap.size();  bpmCnt ) {
		[[unlikely]] case 0:
			N.bpmList[0].base = offset;
			break;
		[[likely]] case 1:
			N.bpmList[0] = { 0, fabs( doubleMap.begin()->second ), offset };
			break;
		default:
			N.bpmList.clear();
			N.bpmList.reserve( bpmCnt );
			for( const auto& [init, value] : doubleMap )
				N.bpmList.push_back({ init, value });
			N.bpmList[0] = { 0, fabs( N.bpmList[0].value ), offset };
			N.bpmList.back().value = fabs( N.bpmList.back().value );

			for( size_t i = 1;  i < bpmCnt;  ++i )
				if( auto last = N.bpmList[i-1], &thiz = N.bpmList[i];  last.value < 0 )   // Linear BPM
					thiz.base = fabs( thiz.value ) + last.value,   // Delta BPM
					thiz.base = last.base + (thiz.init - last.init) * 60000 / thiz.base
										  * log( 1 + thiz.base / -last.value );
				else [[likely]]
					thiz.base = last.base + (thiz.init - last.init) * 60000 / last.value;
	}

	// Provide Metatables
	if( luaL_newmetatable(L, N_WISH) )
		lua_pushcfunction(L, wishGetInfo),		lua_setfield(L, -2, LCALL);
	if( luaL_newmetatable(L, N_HELPER) )
		lua_pushcfunction(L, wishGetInfo),		lua_setfield(L, -2, LCALL),
		lua_pushcfunction(L, freeHelper),		lua_setfield(L, -2, LGC);
	return lua_pushboolean(L, true), 1;
}

int Ar::SetDelta(lua_State* L) noexcept {
	/* Example:
	 * Delta {
	 *     {0},			1,			-- Bar 0, Ratio: 1
	 *     {2, 1/32},	-1,			-- Bar 2, then 1/32 Tone, Ratio: -1
	 *     {2, -1},		0.9,		-- Bar 2, then 1/16 Tone, Ratio: 0.9
	 *     -15,			1,			-- Bar 2(Cached), then 15/16 Tone, Ratio: 1
	 *     ···
	 * }
	 */
	if(! lua_istable(L, 1) )
		return lua_pushboolean(L, false), lua_pushfstring(L, NOT_A_TABLE, DELTA), 2;
	doubleMap.clear();

	for( size_t len = lua_objlen(L, 1),  i = 1;  i < len;  i += 2 )
		doubleMap[ beatToMs(( lua_rawgeti(L, 1, i), checkTime(L, -1) )) ]
			  = ( lua_rawgeti(L, 1, i+1), lua_tonumber(L, -1) ),
		lua_pop(L, 2);

	switch( auto SZ = doubleMap.size();  SZ ) {
		[[unlikely]] case 0:
			return lua_pushboolean(L, true), 1;
		[[unlikely]] case 1:
			N.deltas[0].value = fmax(0, doubleMap.cbegin()->second);
			return lua_pushboolean(L, true), 1;
		[[likely]] default:
			N.deltas.clear();
			N.deltas.reserve(SZ);
			for( const auto [ms, ratio] : doubleMap )
				if( N.deltas.empty()  ||  ratio != N.deltas.back().value )
					N.deltas.push_back({ (uint64_t)(ms/4) * 4.0, ratio });

			if( N.deltas.front().value < 0 )
				N.deltas.front().value = 1;
			if( N.deltas.back().value < 0 )
				N.deltas.back().value = 1;
			N.deltas[0].init = 0;

			if( SZ = N.deltas.size(),  SZ > 131071 ) [[unlikely]]
				N.deltas = {{ 0, 1 }};
			else for( size_t i = 1;  i < SZ;  ++i ) {
				const auto  lastNode = N.deltas[i-1];
					  auto& thisNode = N.deltas[i];
				thisNode.base = lastNode.base + (thisNode.init - lastNode.init) * lastNode.value;

				if( thisNode.base < 0  ||  thisNode.base > 1048575 * (8 - 1.0/1024) ) [[unlikely]]
					return N.deltas.clear(),  N.deltas.push_back({ 0, 1 }),
						   lua_pushboolean(L, false),  lua_pushfstring(L, TIME_OUT_OF_RANGE, DELTA), 2;
			}
			return lua_pushboolean(L, true), 1;
	}
}

int Ar::NewWish(lua_State* L) noexcept {
	/* Example:
	 * local myWish = Wish {		-- When failed, a nil will be returned.
	 *     WithDt = true,			-- true by default
	 *     Special = true,			-- false by default
	 *     {1}, 4, 3, LINEAR,		-- Bar 1, X=4, Y=3, Linear Ease
	 *
	 *     -- Add Radius(5 here) & Degree(0 here) like this
	 *     -12, oldWish {1,-12}.x, oldWish {1,-12}.y, {5, 0, LINEAR},
	 *     ···
	 * }
	 */
	if(! lua_istable(L, 1) )
		return lua_pushnil(L), lua_pushfstring(L, NOT_A_TABLE, WISH), 2;
	N4::Wish W = {
		.isSpecial = (uint8_t)( lua_getfield(L, 1, N_SPECIAL), lua_toboolean(L,-1) ),
		.withDt = (uint8_t)( lua_getfield(L, 1, "WithDt"),  lua_isnil(L,-1) ? true : lua_toboolean(L,-1) )
	};
	if( lua_pop(L, 2),  fillnEmpty(L)  ||  nodeMap.rbegin()->first <= nextDouble( nodeMap.begin()->first ) )
		return lua_pushnil(L), lua_pushstring(L, NOT_SUF), 2;

	W.nodes.reserve( nodeMap.size() );
	for( const auto& [_, node] : nodeMap )
		W.nodes.push_back(node);
	W.nodes.back().ease = STATIC;

	lua_pushlightuserdata(L, ( N.wishes.push_back(W), &N.wishes.back() ));
	luaL_getmetatable(L, N_WISH);
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
	const auto  W = new(lua_newuserdata( L, sizeof(N4::Wish) )) N4::Wish {};
		  auto& nodes = W -> nodes;
	if( fillnEmpty(L) )
		return W->~Wish(), lua_pushnil(L), lua_pushstring(L, NOT_SUF), 2;   // Let Lua GC free the mem of W

	nodes.reserve( nodeMap.size() );
	for( const auto& [_, node] : nodeMap )
		nodes.push_back(node);
	nodes.back().ease = STATIC;

	luaL_getmetatable(L, N_HELPER);
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
	 *     {1, -1}, 2, 3, 4, ···	-- Times
	 * }
	 */
	if( N.wishes.empty() )
		return lua_pushboolean(L, false), lua_pushfstring(L, NO_WISH, CHILD, "WishChild"), 2;
	if(! lua_istable(L, 1) )
		return lua_pushboolean(L, false), lua_pushfstring(L, NOT_A_TABLE, CHILD), 2;

	auto W = &N.wishes.back();
	if( lua_getfield(L, 1, WISH), lua_getmetatable(L, -1), luaL_getmetatable(L, N_WISH),
		lua_equal(L, -1, -2) )
		W = (N4::Wish*)lua_touserdata(L, 2);
	double radius = ( lua_getfield(L, 1, RADIUS), lua_tonumber(L, -1) );
		   radius = radius ? (radius > 7.75 ? 7.75 : radius) : 7;
	double initLoop = ( lua_getfield(L, 1, INITLOOP),  lua_isnil(L,-1) ? 0.25 : lua_tonumber(L,-1) );
		   initLoop = initLoop < 0 ? 0  :  initLoop > 1 ? 1  :  initLoop;
	double deltaLoop = ( lua_getfield(L, 1, DELTALOOP), lua_tonumber(L, -1) );
		   deltaLoop = deltaLoop < -1.875 ? -1.875  :  deltaLoop > 1.875 ? 1.875  :  deltaLoop;
	const bool hintSpecial = ( lua_getfield(L, 1, N_SPECIAL), lua_toboolean(L, -1) );
	lua_pop(L, 4);

	const double minBeat = W->nodes.front().beat,
				 maxBeat = W->nodes.back().beat;
	const size_t len = lua_objlen(L, 1);			W->wishChilds.reserve( len ),  W->hints.reserve( len );
	for( size_t i = 1;  i <= len;  ++i )
		if( const double beat = (lua_rawgeti(L, 1, i), checkTime(L, -1));  lua_pop(L, 1),  beat > minBeat )
			W->wishChilds.push_back({ beat, radius, initLoop, deltaLoop, hintSpecial }),
			(beat <= maxBeat) ? W->hints.push_back({ beat, hintSpecial }) : (void)0 ;
	return lua_pushboolean(L, true), 1;
}

int Ar::NewHint(lua_State* L) noexcept {
	/* Example:
	 * Hint {
	 *     Wish = myWish,			-- The last Wish of the Fumen by default
	 *     Special = false,			-- False by default
	 *     {1}, 1, 2, 3, 4, ···		-- Times
	 * }
	 */
	if( N.wishes.empty() )
		return lua_pushboolean(L, false), lua_pushfstring(L, NO_WISH, HINT, HINT), 2;
	if(! lua_istable(L, 1) )
		return lua_pushboolean(L, false), lua_pushfstring(L, NOT_A_TABLE, HINT), 2;

	auto W = &N.wishes.back();
	if( lua_getfield(L, 1, WISH), lua_getmetatable(L, -1), luaL_getmetatable(L, N_WISH),
		lua_equal(L, -1, -2) )
		W = (N4::Wish*)lua_touserdata(L, 2);
	const bool hintSpecial = ( lua_getfield(L, 1, N_SPECIAL), lua_toboolean(L, -1) );
	lua_pop(L, 1);

	const double minBeat = W->nodes.front().beat,
				 maxBeat = W->nodes.back().beat;
	const size_t len = lua_objlen(L, 1);			W->hints.reserve( len );
	for( size_t i = 1;  i <= len;  ++i )
		if( const double beat = ( lua_rawgeti(L, 1, i), checkTime(L, -1) );  lua_pop(L, 1),
			beat > minBeat  &&  beat <= maxBeat )
			W->hints.push_back({ beat, hintSpecial });
	return lua_pushboolean(L, true), 1;
}

int Ar::NewEcho(lua_State* L) noexcept {
	/* Example:
	 * Echo {
	 *     Radius = 7.0,			-- 0 by Default
	 *     Special = false,			-- Scored if true. false by default
	 *     InitLoop = 0.25,			-- 0.25 by default, ignored if Radius is 0
	 *     DeltaLoop = 1.25,		-- 0 by default, ignored if Radius is 0
	 *     {1}, 8, 0.5,				-- T1, X1, Y1
	 *     -12, 8, 0.5,				-- T2, X2, Y2
	 *     ···
	 * }
	 */
	if(! lua_istable(L, 1) )
		return lua_pushboolean(L, false), lua_pushfstring(L, NOT_A_TABLE, ECHO), 2;
	const bool isSpecial = (lua_getfield(L, 1, N_SPECIAL), lua_toboolean(L, -1));
		   double radius = (lua_getfield(L, 1, RADIUS), lua_tonumber(L, -1)), initLoop = 0, deltaLoop = 0;
				  radius = radius > 7.75 ? 7.75  :  radius < 0 ? 0  :  radius;
	lua_pop(L, 2);

	if( radius )
		initLoop = ( lua_getfield(L, 1, INITLOOP), lua_tonumber(L, -1) ),
		initLoop = initLoop ? (initLoop < 0 ? 0  :  initLoop > 1 ? 1  :  initLoop) : 0.25,
		deltaLoop = ( lua_getfield(L, 1, DELTALOOP), lua_tonumber(L, -1) ),
		deltaLoop = deltaLoop < -1.875 ? -1.875  :  deltaLoop > 1.875 ? 1.875  :  deltaLoop,
	/**/lua_pop(L, 2);

	const size_t inputLen = lua_objlen(L, 1);			N.echoes.reserve(inputLen);
	for( size_t i = 1;  i < inputLen;  i += 3 ) {
		const double beat = ( lua_rawgeti(L, 1, i), checkTime(L, -1) ),
					 x = ( lua_rawgeti(L, 1, i+1), lua_tonumber(L, -1) ),
					 y = ( lua_rawgeti(L, 1, i+2), lua_tonumber(L, -1) );
		N.echoes.push_back({ .x = x < -23.9375 ? -23.9375  :  x > 39.9375 ? 39.9375  :  x,
							 .y = y <   -3.875 ?   -3.875  :  y >  11.875 ?  11.875  :  y,
							 beat, radius, initLoop, deltaLoop, isSpecial });
	}
	return lua_pushboolean(L, true), 1;
}

int Ar::NewVerse(lua_State* L) noexcept {
	/* Usage:
	 * Verse(since_time) do ... end
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
				e.x = 16 - e.x,			e.initLoop = (e.initLoop > 0.5 ? 1.5 : 0.5) - e.initLoop,
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
				e.y = 8 - e.y,			e.initLoop = 1 - e.initLoop,
										e.deltaLoop = -e.deltaLoop;
			break;
		default:   // Mirror LR & UD
			for( auto& w : std::span(N.wishes).subspan(N.verseWidx) ) {
				for( auto& node : w.nodes )
					node.x = 16 - node.x,  node.y = 8 - node.y;
				for( auto& child : w.wishChilds )
					child.initLoop = child.initLoop + (child.initLoop > 0.5 ? -0.5 : 0.5);
			}
			for( auto& e : std::span(N.echoes).subspan(N.verseEidx) )
				e.x = 16 - e.x,  e.y = 8 - e.y,
				e.initLoop = e.initLoop + (e.initLoop > 0.5 ? -0.5 : 0.5);
	}   return 0;
}

int Ar::SinceTone(lua_State* L) noexcept {
	/* Usage:
	 * local since_tone_or_nil = SinceTone(set_to_or_nil)
	 */
	if(! lua_isnumber(L, 1) )
		return lua_pushnumber(L, N.sinceTone), 1;
	if( const double sinceTone = lua_tonumber(L, 1);  sinceTone >= 0 )
		N.sinceTone = sinceTone;
	return 0;
}

int Ar::ConvTime(lua_State* L) noexcept {
	/* Usage:
	 * local result = Arf4.ConvTime(in, mode)   -- [0] toTone  [1] barToMs  [2] toneToMs
	 */
	lua_Number T = lua_tonumber(L, 1);
	if( const auto mode = lua_tointeger(L, 2);  mode )
		return lua_pushnumber(L, beatToMs(toneToBeat(  mode == 1 ? barToTone(T) : fmax(T,0)  ))), 1;
	return lua_istable(L, 1)  ?  checkTime(L, 1), lua_rawgeti(L, 1, 2), T = lua_tonumber(L, -1)  :  0,
		   lua_pushnumber( L, N.sinceTone + (T<0 ? -T/16 : T) ), 1;
}


/* Arf Compile Fn */
static std::map<uint64_t, int16_t> valueMap;
static std::vector< std::vector<Arf4::Wish> > idxProto;
int Ar::OrganizeArf(lua_State* L) noexcept {
	/* Usage:
	 * local before_or_false, objcnt, wgo_required, hgo_required, ego_required = Arf4.OrganizeArf()
	 */
	#define FIX(a,b)  ( (a)<(b) ? -0.5 : 0.5 )
	Fumen F = { .val = 0 };

	// Organize Deltas
	F.deltas.reserve( N.deltas.size() + 1 ), F.deltas.push_back({ .val = 1 });
	for( const auto [init, value, base] : N.deltas )
		if( init < 1048576 )   /* Arf4.h */
			F.deltas.push_back({ .t = (uint64_t)init >> 2,
								 .absV = (uint64_t)( fmin( abs(value), 8 - 1.0/1024 ) * 1024 ),
								 .base = (uint64_t)( base * 1024 ) });
		else return lua_pushboolean(L, false), lua_pushfstring(L, NEMESIS_TIME_OOR, "DeltaNode"), 2;

	// Organize Echoes
	valueMap.clear();
	for( const auto [x, y, beat, radius, initLoop, deltaLoop, isSpecial] : N.echoes )
		if( const uint64_t ms = beatToMs(beat);  ms < 637  ||  ms > 1048575 - 470 )   /* Arf4.h */
			return lua_pushboolean(L, false), lua_pushfstring(L, NEMESIS_TIME_OOR, ECHO), 2;
		else if( const Body baseEcho = { .cdx = (int64_t)( FIX(x,8) + (x-8) * 16 ),		.ms = ms,
										 .cdy = (int64_t)( FIX(y,4) + (y-4) * 8 ),	 // .status = 0,
										 .radius = (uint64_t)( 0.5 + radius * 4 ),	 // .deltaMs = 0,
										 .initLoop = (uint64_t)( 0.5 + initLoop * 64 ),
										 .deltaLoop = (int64_t)( FIX(deltaLoop,0) + deltaLoop * 8 ) };
		valueMap[baseEcho.val] == false ) [[likely]]   // Insertion and Value Checking, in one sentence
			valueMap[baseEcho.val] = isSpecial;

	const size_t echoSize = valueMap.size();
	if( echoSize > 131072 )   /* inout.cpp 1/8 */
		return lua_pushboolean(L, false), lua_pushfstring(L, NEMESIS_SLE, "Echoes", LI 131072), 2;
	/**/F.echoes.reserve( echoSize );
	for( Body e;  const auto [val, isSpecial] : valueMap )
		F.echoes.push_back(( e.val = val,  e.status = isSpecial,  e ));
	std::ranges::sort( F.echoes, [](const Body a, const Body b) { return  a.val << 27  <  b.val << 27; } );

	// Organize Times of Wishes, Add Hints into `F.hints`
	valueMap.clear();
	for( auto& w : N.wishes ) {
		for( auto& n : w.nodes )
			if( (n.beat = beatToMs( n.beat )) > 1048575 )   /* Arf4.h */
				return lua_pushboolean(L, false), lua_pushfstring(L, NEMESIS_TIME_OOR, "Wish Node"), 2;
		if( w.nodes.size() > 4096 )   /* Arf4.h */
			return lua_pushboolean(L, false), lua_pushfstring(L, NEMESIS_SLE, "a Wish's Nodes", LI 4096), 2;

		if( w.withDt )   // Use valueMap to deduplicate & sort childs later
			for( auto& c : w.wishChilds )		{  c.beat = msToDt( beatToMs(c.beat) );  }
		else
			for( auto& c : w.wishChilds )		{  c.beat = beatToMs(c.beat);			 }

		for( auto [beat, isSpecial] : w.hints )
			if( beat = beatToMs(beat),  beat >= 510 )
				if( const Body baseHint = { .ms = (uint64_t)beat,
											.cdx = (int64_t)( wishCacheT(w, beat),
															  FIX(w.wX,8) + (w.wX-8) * 16 ),
											.cdy = (int64_t)( FIX(w.wY,4) + (w.wY-4) * 8 ) };
				valueMap[baseHint.val] == false )
					valueMap[baseHint.val] = isSpecial;
	}

	const size_t hintSize = valueMap.size();
	if( hintSize > 32767 )   /* inout.cpp 2/8 */
		return lua_pushboolean(L, false), lua_pushfstring(L, NEMESIS_SLE, "Hints", LI 32767), 2;
	/**/F.hints.reserve( hintSize );
	for( Body h;  const auto [val, isSpecial] : valueMap )
		F.hints.push_back(( h.val = val,  h.status = isSpecial,  h ));

	/* Generate hIdx & eIdx, Count scored objects
	 * Metadata: before, objectCount, hgoRequired, egoRequired
	 */
	int16_t iSize = (   /* inout.cpp "hIdx" 3/8 */
		F.hints.empty()  ?  -1 : (F.before = F.hints.back().ms + 470) >> 10   // 1st time to assign F.before
	) + 1;
	idxProto.clear(), idxProto.resize( iSize );

	for( size_t i = 0;  i < hintSize;  ++i ) {
		const Body h = F.hints[i];
		if( F.sHit < 31 )		F.sHit += h.status;
		else [[unlikely]]		F.hints[i].status = NJUDGED;

		const size_t endGroup = (h.ms + 470) >> 10;
		for( size_t group = (h.ms - 510) >> 10;  group <= endGroup;  ++group )
			idxProto[group].push_back({ .val = i });
	}

	F.idx.reserve( iSize );
	for( const auto& group : idxProto )
		if( uint32_t since, count;  group.empty() )
			F.idx.push_back({ .hSince = hintSize });
		else if( since = group[0].val,  count = group.back().val - since + 1,  count > 511 ) [[unlikely]]
			return lua_pushboolean(L, false),
				   lua_pushfstring(L, NEMESIS_SLE, "Hints within 1024ms", LI 511), 2;   /* Arf4.h */
		else if( F.idx.push_back({ .hSince = since }),  count > F.hgoRequired )
			F.hgoRequired = count;

	const int16_t eiSize = (   /* inout.cpp "eIdx" 4/8 */
		F.echoes.empty()  ?  -1 : (F.before = fmax( F.before, F.echoes.back().ms + 470 )) >> 10
	) + 1;
	idxProto.clear(), idxProto.resize( eiSize );

	F.objectCount = hintSize;
	for( size_t i = 0;  i < echoSize;  ++i ) {
		const Body e = F.echoes[i];
		if( F.objectCount  &&  !(F.objectCount += e.status) )   /* Arf4.h */
			return lua_pushboolean(L, false), lua_pushfstring(L, NEMESIS_SLE, "Objects", LI 32767), 2;

		int32_t initMs = e.ms - (e.radius ? 1011 : 637);
		if( initMs < 0 )
			initMs = 0;

		const size_t endGroup = (e.ms + 470) >> 10;
		for( size_t group = initMs >> 10;  group <= endGroup;  ++group )
			idxProto[group].push_back({ .val = i });
	}

	while( eiSize > iSize )
		F.idx.push_back({ .hSince = hintSize }),  ++iSize;
	for( int16_t i = -1;  const auto& group : idxProto )
		if( uint32_t since, count;  ++i, group.empty() )
			F.idx[i].eSince = echoSize;
		else if( since = group[0].val,  count = group.back().val - since + 1,  count > 511 ) [[unlikely]]
			return lua_pushboolean(L, false),
				   lua_pushfstring(L, NEMESIS_SLE, "Echoes within 1024ms", LI 511), 2;   /* Arf4.h */
		else if( F.idx[i].eSince = since,  count > F.egoRequired ) [[likely]]
			F.egoRequired = count;

	/* Flatten Nodes & WishChilds
	 * Metadata: before
	 */
	F.nodes.reserve(32767),  F.wishChilds.reserve(32767);
	std::ranges::sort( N.wishes, [](auto& a, auto& b) {  return a.nodes[0].beat < b.nodes[0].beat;  } );
	idxProto.clear(),  idxProto.resize(511);
	
	for( auto& w : N.wishes ) {
		Wish wView = { .val = 0 };
			 wView.nSince = F.nodes.size(),  wView.isSpecial = w.isSpecial,  wView.cIndex = 1;

		// Organize Nodes
		if( const auto wNs = w.nodes.size();  wView.nSince + wNs + (wNs>2) > 262144 )   /* inout.cpp 5/8 */
			return lua_pushboolean(L, false), lua_pushfstring(L, NEMESIS_SLE, "Wish Nodes", LI 262144), 2;
		for( const auto [x, y, beat, radius, degree, ease] : w.nodes )
			F.nodes.push_back({ .cdx = (int64_t)( FIX(x,8) + (x-8) * 16 ),   .ms = (uint64_t)beat,
								.cdy = (int64_t)( FIX(y,4) + (y-4) * 8 ),	 .ease = ease,
								.radius = (uint64_t)( 0.5 + radius * 4 ),
								.deg = (int64_t)( FIX(degree,0) + degree) });
		wView.nType = w.nodes.size() > 2  ?  (  F.nodes.push_back({ .val = 0 }),  1  ) : 0;

		// Organize WishChilds
		valueMap.clear();
		for(Child c;  const auto& nC : w.wishChilds)
			c = { .radius = (uint64_t)( 0.5 + nC.radius * 4 ),
				  .initLoop = (uint64_t)( 0.5 + nC.initLoop * 64 ),
				  .deltaLoop = (int64_t)( FIX(nC.deltaLoop,0) + nC.deltaLoop * 8 ),
				  .zDt = (uint64_t)( nC.beat * 1024 ) },
			valueMap[c.val] = 0;
		if( const size_t childCnt = valueMap.size();  childCnt > 8192 ) [[unlikely]]   /* Arf4.h */
			return lua_pushboolean(L, false),
				   lua_pushfstring(L, NEMESIS_SLE, "a Wish's WishChilds", LI 8192), 2;
		else if( F.wishChilds.size() + childCnt + !!childCnt > 131072 ) [[unlikely]]   /* inout.cpp 6/8 */
			return lua_pushboolean(L, false), lua_pushfstring(L, NEMESIS_SLE, "WishChilds", LI 131072), 2;
		else if( childCnt ) [[likely]]  {
			for(const auto [cVal, _]  :  wView.cSince = F.wishChilds.size(), valueMap)
				F.wishChilds.push_back({ .val = cVal });

			// Calculate wgoRequired for this Wish (Here the `.cIndex` field is borrowed)
			doubleMap.clear();
			constexpr double LOWEST_SPEED_RCP = 1500.0 / 11;
			for( const auto& c : w.wishChilds )
				/* From */  doubleMap[ fmax(0, c.beat - fmax(c.radius, 6) * LOWEST_SPEED_RCP) ] += 1,
				/*  To  */  doubleMap[ c.beat ] -= 1;
			for( int16_t currentStep = 1 /* the Wish itself */ ;  const auto [_, stepDelta] : doubleMap )
				if( (currentStep += stepDelta) > wView.cIndex )
					wView.cIndex = currentStep;
			wView.cType = 1,  wView.withDt = w.withDt,  F.wishChilds.push_back({ .val = 0 });
		}

		/* Push this Wish into idxProto
		 * Update F.before
		 */
		const uint32_t lastMs = w.nodes.back().beat,  endGroup = lastMs >> 11;
		for( uint32_t i = (uint32_t)w.nodes.front().beat >> 11;  i <= endGroup;  ++i )
			idxProto[i].push_back( wView );
		F.before = lastMs > F.before ? lastMs : F.before;
	}

	for(const int16_t fSize = (F.before >> 10) + 1;  iSize < fSize;  ++iSize)
		F.idx.push_back({ .hSince = hintSize,  .eSince = echoSize });
	idxProto.resize( (F.before >> 11) + 1 );   /* inout.cpp "wIdx" 7/8 */

	/* Flatten Wishes
	 * Metadata: wgoRequired
	 */
	F.wishes.reserve(65535);
	for( int16_t i = -1;  auto& group : idxProto )
		if( const uint16_t groupSize = group.size();  ++i,  !groupSize ) [[unlikely]]
			F.idx[i].wSince = 16777215;
		else if( uint16_t groupWgoUsed = 0;  groupSize > 1023 ) [[unlikely]]   /* Arf4.h */
			return lua_pushboolean(L, false),
				   lua_pushfstring(L, NEMESIS_SLE, WISHES_2048, LI 1023), 2;
		else [[likely]] {
			F.idx[i].wSince = F.wishes.size();
			for( auto wish : group )
				groupWgoUsed += wish.cIndex,  wish.cIndex = 0 /* End of the borrowing */,
				F.wishes.push_back( wish );
			if( F.wishes.size() > 16777215 )   /* inout.cpp 8/8 */
				return lua_pushboolean(L, false), lua_pushfstring(L, NEMESIS_SLE, "Wishes", LI 16777215), 2;
			if( groupWgoUsed > 1023 )   /* Arf4.h */
				return lua_pushboolean(L, false),
					   lua_pushfstring(L, NEMESIS_SLE, WISHES_2048, LI 1023), 2;
			if( groupWgoUsed > F.wgoRequired )
				F.wgoRequired = groupWgoUsed;
		}
	for( const auto wCnt = F.wishes.size();  auto& idx : F.idx )
		idx.wSince = idx.wSince < 16777215 ? idx.wSince : wCnt;

	return  Arf = { .val = F.val },
			F.deltas.swap(Arf.deltas),				F.nodes.swap(Arf.nodes),
			F.echoes.swap(Arf.echoes),				F.wishes.swap(Arf.wishes),
			F.hints.swap(Arf.hints),				F.wishChilds.swap(Arf.wishChilds),
			F.idx.swap(Arf.idx),					lua_pushinteger(L, F.before),
			lua_pushinteger(L, F.objectCount),		lua_pushinteger(L, F.wgoRequired),
			lua_pushinteger(L, F.hgoRequired),		lua_pushinteger(L, F.egoRequired),
	5;
}
#endif