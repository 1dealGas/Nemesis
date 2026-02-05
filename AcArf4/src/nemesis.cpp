// Nemesis, the Aerials Fumen Compiler. //
#ifdef AR_BUILD_VIEWER
#include <algorithm>
#include <Arf4.h>
#include <map>

/* Strings & Error Reasons */
static constexpr auto
	RADIUS = "Radius", INITLOOP = "InitLoop", DELTALOOP = "DeltaLoop",
	WISHES_2048 = "Wishes within 2048ms", WISH = "Wish", HINT = "Hint", ECHO = "Echo", CHILD = "Child",
	N_WISH = "NEMESIS_WISH",  N_HELPER = "NEMESIS_HELPER", N_SPECIAL = "Special", N_DEL = "Del",
	LCALL = "__call", LADD = "__add";
static constexpr auto
	NOT_SUF = "Count of Node(s) is not sufficient to create a Wish / Helper",
	NEMESIS_TIME_OOR = R"(Nemesis Compiler: Time(ms) of %s out of range)",
	NEMESIS_SLE = R"(Nemesis Compiler: Count limit of %s(%td) exceeded)",
	NOT_A_TABLE = R"(API "%s" requires a Lua Table for the Arg 1)";
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

	/* Wish */
	struct Point {
		double		x, y, beat, radius, degree;
		uint8_t		ease;
	};
	struct Child {
		double		beat;
		double		radius, initLoop, deltaLoop;
	};
	struct Wish {
		std::vector<Point>	nodes;
		std::vector<Child>	wishChilds;
		std::vector<Hint>	manualHints;
		uint16_t			nIdx, isSpecial:8, withCs:8;
		//------------------------//
		float				wRadius;
		double				wX, wY, wNx, wNy, wDegree;
	};
}
static struct {
	std::vector<N4::Tempo>	tempoList = {{ .a=4, .b=4 }};
	std::vector<N4::Delta>	bpmList = {{ 0, 170 }};
	std::vector<N4::Wish>	wishes;
	std::vector<N4::Echo>	echoes;
	uint32_t				bIdx, tIdx;
	double					sinceTone;
}	/**/					N;

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

static double nextFval(const double d) noexcept {   // Little Endian Only
	union{ double d; uint64_t u; }  nd = {d};
	return ++nd.u, nd.d;
}

static void wishCacheT(N4::Wish& w, const double beat) noexcept {
	if( const auto& init = w.nodes.front();  beat < init.beat ) [[unlikely]]
		w.wNx = init.x, w.wNy = init.y, w.wRadius = init.radius, w.wDegree = init.degree;
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
		return S = barToTone(( lua_rawgeti(L, where, 1), lua_tonumber(L, -1) )),			lua_pop(L, 1),
			   T = ( lua_rawgeti(L, where, 2), lua_tonumber(L, -1) ),						lua_pop(L, 1),
			   toneToBeat( (local ? S : N.sinceTone = S) + (T<0 ? -T/16 : T) );
	else [[likely]]
		return T = lua_tonumber(L, where),
			   toneToBeat( N.sinceTone + (T<0 ? -T/16 : T) );
}   // Stack balanced; Won't pop

static N4::Point checkPointArg(lua_State* L, const int where) noexcept {   // Stack balanced; Won't pop
	if( lua_Number r, d, e;  lua_istable(L, where) )
		return  r = ( lua_rawgeti(L, where, 1), lua_tonumber(L, -1) ),						lua_pop(L, 1),
				d = ( lua_rawgeti(L, where, 2), lua_tonumber(L, -1) ),						lua_pop(L, 1),
				e = ( lua_rawgeti(L, where, 3), lua_tointeger(L, -1) ),						lua_pop(L, 1),
				N4::Point { .radius = r < 0 ? 0  :  r > 15.75 ? 15.75  :  r,
							.degree = d < -1024 ? -1024  :  d > 1023 ? 1023  :  d,
							.ease = (uint8_t)(e > Arf4::OUTSINE ? Arf4::LINEAR : e) };
	const auto e = lua_tointeger(L, where);
	return { .ease = (uint8_t)(e > Arf4::OUTSINE  ?  Arf4::LINEAR : e) };
}

static std::map<double, N4::Point> nodeMap;
static bool fillnEmpty(lua_State* L, const uint8_t where) noexcept {   // `where` must be positive
	nodeMap.clear();
	for( size_t len = lua_objlen(L, where),  i = 1;  i < len;  i += 4 ) {
		const double beat = ( lua_rawgeti(L, where, i), checkTime(L, -1) ),
						x = ( lua_rawgeti(L, where, i+1), lua_tonumber(L, -1) ),
						y = ( lua_rawgeti(L, where, i+2), lua_tonumber(L, -1) );
		auto point = ( lua_rawgeti(L, where, i+3), checkPointArg(L, -1) );
			 point.x = x < -23.9375 ? -23.9375  :  x > 39.9375 ? 39.9375  :  x;
			 point.y = y <   0.0625 ?   0.0625  :  y >  7.9375 ?  7.9375  :  y;
			 point.beat = beat;
		nodeMap[( nodeMap.contains(beat) ? nextFval(beat) : beat )] = point;
		lua_pop(L, 4);
	}
	return nodeMap.empty();
}

static int wishAdd(lua_State* L) noexcept {
	if( const auto w = lua_islightuserdata(L,1) ? &N.wishes[ (size_t)lua_touserdata(L,1) ]
												: (N4::Wish*)lua_touserdata(L,1);
	/**/lua_istable(L,2)  &&  !fillnEmpty(L,2) ) {
		for( const auto node : w->nodes )
			nodeMap.insert({ node.beat, node });
		for( uint16_t i = 0;  const auto [_, node] : w->nodes.resize(nodeMap.size()), nodeMap )
			w->nodes[i++] = node;
	}	return lua_settop(L,1), 1;
}

static int wishGetInfo(lua_State* L) noexcept {
	const auto w = lua_islightuserdata(L,1) ? &N.wishes[ (size_t)lua_touserdata(L,1) ]
											: (N4::Wish*)lua_touserdata(L,1);		// [1] Wish / Helper
	wishCacheT( *w, checkTime(L, 2, true) );										// [2] Time
	lua_createtable(L, 0, 8);														// [3]

	return  lua_pushnumber(L, w->wX),					lua_setfield(L, 3, "x"),
			lua_pushnumber(L, w->wY),					lua_setfield(L, 3, "y"),
			lua_pushnumber(L, w->wNx),					lua_setfield(L, 3, "node_x"),
			lua_pushnumber(L, w->wNy),					lua_setfield(L, 3, "node_y"),
			lua_pushnumber(L, w->wRadius),				lua_setfield(L, 3, "radius"),
			lua_pushnumber(L, w->wDegree),				lua_setfield(L, 3, "degree"),
			lua_pushnumber(L, w->nodes.back().beat),	lua_setfield(L, 3, "to_beat"),
			lua_pushnumber(L, w->nodes[0].beat),		lua_setfield(L, 3, "from_beat"),  1;
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
		return lua_pushboolean(L, false),
			   lua_pushfstring(L, NOT_A_TABLE, "Time"), 2;
	const double offset = (lua_getfield(L, 1, "Offset"), fmax( lua_tonumber(L,-1), 1 ));
		lua_getfield(L, 1, "Tempo");
	N = {};

	// Tempo
	tempoMap.clear();
	if( size_t tempoInputLen;  lua_istable(L, 3)  &&  (tempoInputLen = lua_objlen(L, 3)) > 2 )
		for( size_t i = 1;  i < tempoInputLen;  i += 3 ) {   // [1] Args Table  [2] Offset  [3] Tempo Table
			double bar = ( lua_rawgeti(L, 3, i), lua_tonumber(L, -1) );
				   bar = bar < 0 ? 0 : bar;
			const uint32_t a = ( lua_rawgeti(L, 3, i+1), lua_tointeger(L, -1) ),
						   b = ( lua_rawgeti(L, 3, i+2), lua_tointeger(L, -1) );
			tempoMap[bar] = { .bar = bar,  .a = a ? a : 4,  .b = b ? b : 4 };
			lua_pop(L, 3);
		}

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
		N.tempoList.clear();   // No need to reserve

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
			N.bpmList.clear();   // No need to reserve
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
		lua_pushcfunction(L, wishAdd),			lua_setfield(L, -2, LADD),
		lua_pushcfunction(L, wishGetInfo),		lua_setfield(L, -2, LCALL);
	if( luaL_newmetatable(L, N_HELPER) )
		lua_pushcfunction(L, wishAdd),			lua_setfield(L, -2, LADD),
		lua_pushcfunction(L, wishGetInfo),		lua_setfield(L, -2, LCALL),
		lua_pushcfunction(L, freeHelper),		lua_setfield(L, -2, LGC);
	return lua_pushboolean(L, true), 1;
}

int Ar::NewWish(lua_State* L) noexcept {
	/* Example:
	 * local myWish = Wish {		-- When failed, a nil will be returned.
	 *     WithCs = true,			-- true by default
	 *     Special = true,			-- false by default
	 *     {1}, 4, 3, LINEAR,		-- Bar 1, X=4, Y=3, Linear Ease
	 *
	 *     -- Add Radius(5 here) & Degree(0 here) like this
	 *     -12, oldWish {1,-12}.x, oldWish {1,-12}.y, {5, 0, LINEAR},
	 *     ···
	 * }
	 */
	if(! lua_istable(L, 1) )
		return lua_pushnil(L),
			   lua_pushfstring(L, NOT_A_TABLE, WISH), 2;
	if( fillnEmpty(L,1)  ||  nodeMap.rbegin()->first <= nextFval( nodeMap.begin()->first ) )
		return lua_pushnil(L),
			   lua_pushstring(L, NOT_SUF), 2;
	N4::Wish& W = N.wishes.emplace_back( N4::Wish {
		.isSpecial = (uint8_t)( lua_getfield(L, 1, N_SPECIAL), lua_toboolean(L,-1) ),
		.withCs = (uint8_t)( lua_getfield(L, 1, "WithCs"),  lua_isnil(L,-1) ? true : lua_toboolean(L,-1) )
	} );
	W.nodes.reserve( nodeMap.size() );

	for( const auto& [_, node] : nodeMap )
		W.nodes.push_back(node);
	lua_pushlightuserdata(L, (void*)( N.wishes.size() - 1 ));   // Consider Vector Expansion
		luaL_getmetatable(L, N_WISH),  lua_setmetatable(L, -2);
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
		return lua_pushnil(L),
			   lua_pushfstring(L, NOT_A_TABLE, "Helper"), 2;
	if( fillnEmpty(L,1) )
		return lua_pushnil(L),
			   lua_pushstring(L, NOT_SUF), 2;
	auto& W = *new(lua_newuserdata( L, sizeof(N4::Wish) )) N4::Wish {};

	for( const auto& [_, node] : nodeMap )   // Seems okay not to reserve
		W.nodes.push_back(node);
	luaL_getmetatable(L, N_HELPER);
		  lua_istable(L, -1) ? (void)lua_setmetatable(L, -2) : W.~Wish();
	return 1;
}

int Ar::NewChild(lua_State* L) noexcept {
	/* Example:
	 * Child {
	 *     Wish = nil,				-- The last Wish of the Fumen by default
	 *     Radius = 7.0,			-- 7.0 by Default
	 *     InitLoop = 0.25,			-- 0.25 by default
	 *     DeltaLoop = 1.25,		-- 0 by default
	 *     {1, -1}, 2, 3, 4, ···	-- Times
	 * }
	 */
	if(! lua_istable(L, 1) )
		return lua_pushboolean(L, false),
			   lua_pushfstring(L, NOT_A_TABLE, CHILD), 2;
	if(! N.wishes.empty() ) {
		auto W = &N.wishes.back();
		if( lua_getfield(L, 1, WISH),
			lua_getmetatable(L, -1),  luaL_getmetatable(L, N_WISH),
		/**/lua_equal(L, -1, -2) )
			W = &N.wishes[ (size_t)lua_touserdata(L,2) ];

		double radius = ( lua_getfield(L, 1, RADIUS), lua_tonumber(L, -1) );
			   radius = radius ? (radius > 7.75 ? 7.75 : radius) : 7;
		double initLoop = ( lua_getfield(L, 1, INITLOOP),  lua_isnil(L,-1) ? 0.25 : lua_tonumber(L,-1) );
			   initLoop = initLoop > 0 && initLoop < 1  ?  initLoop : 0;
		double deltaLoop = ( lua_getfield(L, 1, DELTALOOP), lua_tonumber(L, -1) );
			   deltaLoop = deltaLoop < -1.875 ? -1.875  :  deltaLoop > 1.875 ? 1.875  :  deltaLoop;

		const size_t len = lua_objlen(L, 1);
			W->wishChilds.reserve( len );
		for( size_t i = 1;  i <= len;  ++i )
			W->wishChilds.push_back({ /* Beat */ ( lua_rawgeti(L,1,i),  checkTime(L,-1) ),
									   radius, initLoop, deltaLoop }),  lua_pop(L,1);
	}	return lua_pushboolean(L, true), 1;
}

int Ar::NewHint(lua_State* L) noexcept {
	/* Example:
	 * Hint {
	 *     Wish = myWish,			-- The last Wish of the Fumen by default
	 *     Special = false,			-- False by default
	 *     {1}, 1, 2, 3, 4, ···		-- Times
	 * }
	 */
	if(! lua_istable(L, 1) )
		return lua_pushboolean(L, false),
			   lua_pushfstring(L, NOT_A_TABLE, HINT), 2;
	if(! N.wishes.empty() ) {
		auto W = &N.wishes.back();
		if( lua_getfield(L, 1, WISH),
			lua_getmetatable(L, -1), luaL_getmetatable(L, N_WISH),
		/**/lua_equal(L, -1, -2) )
			W = &N.wishes[ (size_t)lua_touserdata(L,2) ];
		const bool hintSpecial = ( lua_getfield(L, 1, N_SPECIAL), lua_toboolean(L, -1) );

		for( size_t len = lua_objlen(L, 1),  i = 1;  i <= len;  ++i )   // No need to reserve
			W->manualHints.push_back({ /*Beat*/ ( lua_rawgeti(L, 1, i), checkTime(L, -1) ),  hintSpecial }),
			lua_pop(L, 1);
	}	return lua_pushboolean(L, true), 1;
}

int Ar::NewEcho(lua_State* L) noexcept {
	/* Example:
	 * Echo {
	 *     Radius = 7.0,			-- 0 by Default
	 *     Special = false,			-- Scored if true. false by default
	 *     InitLoop = 0.25,			-- 0.25 by default (0 when Radius is 0)
	 *     DeltaLoop = 1.25,		-- 0 by default (0 when Radius is 0)
	 *     {1}, 8, 0.5,				-- T1, X1, Y1
	 *     -12, 8, 0.5,				-- T2, X2, Y2
	 *     ···						-- Use `Del = true` to remove Echoes by Time & Pos
	 * }
	 */
	if(! lua_istable(L, 1) )
		return lua_pushboolean(L, false),
			   lua_pushfstring(L, NOT_A_TABLE, ECHO), 2;
	const bool isSpecial = (lua_getfield(L, 1, N_SPECIAL), lua_toboolean(L, -1)),
				isDelete = (lua_getfield(L, 1, N_DEL), lua_toboolean(L, -1));
	const auto len = lua_objlen(L, 1);

	if( isDelete )  [[unlikely]]
		return std::erase_if(N.echoes, [=](const N4::Echo& e) {
			for( size_t i = 1;  i < len;  i += 3 )
				if( const double beat = ( lua_rawgeti(L, 1, i), checkTime(L, -1) ),
									x = ( lua_rawgeti(L, 1, i+1), lua_tonumber(L, -1) ),
									y = ( lua_rawgeti(L, 1, i+2), lua_tonumber(L, -1) );	 lua_pop(L,3),
				/**/e.beat == beat  &&  abs(e.x-x) + abs(e.y-y) <= 0.125  &&  (!isSpecial || e.isSpecial) )
					return true;
			return false;
		}), lua_pushboolean(L, true), 1;
	N.echoes.reserve(len);

	double radius = ( lua_getfield(L, 1, RADIUS),  lua_tonumber(L, -1) ),
		   initLoop = 0,  deltaLoop = 0;
	if( radius > 0 )
		radius = radius < 7.75 ? radius : 7.75,
		initLoop = ( lua_getfield(L, 1, INITLOOP), lua_tonumber(L, -1) ),
		initLoop = lua_isnil(L, -1) ? 0.25  :  (initLoop > 0 && initLoop < 1) ? initLoop  :  0,
		deltaLoop = ( lua_getfield(L, 1, DELTALOOP), lua_tonumber(L, -1) ),
		deltaLoop = deltaLoop < -1.875 ? -1.875  :  deltaLoop > 1.875 ? 1.875  :  deltaLoop;

	for( size_t i = 1;  i < len;  i += 3 ) {
		const double beat = ( lua_rawgeti(L, 1, i), checkTime(L, -1) ),
						x = ( lua_rawgeti(L, 1, i+1), lua_tonumber(L, -1) ),
						y = ( lua_rawgeti(L, 1, i+2), lua_tonumber(L, -1) );				 lua_pop(L,3);
		N.echoes.push_back({ .x = x < -23.9375 ? -23.9375  :  x > 39.9375 ? 39.9375  :  x,
							 .y = y <   0.0625 ?   0.0625  :  y >  7.9375 ?  7.9375  :  y,
							 beat, radius, initLoop, deltaLoop, isSpecial });
	}
	return lua_pushboolean(L, true), 1;
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
static std::map<uint64_t, Arf4::Duo> valMap;
static std::vector< std::vector<Arf4::Wish> > idxProto;
int Ar::OrganizeArf(lua_State* L) noexcept {
	/* Usage:
	 * local before_or_false, objcnt, wgo_required, hgo_required, ego_required = Arf4.OrganizeArf()
	 */
	#define FIX(a,b)  ( (a)<(b) ? -0.5 : 0.5 )
	Fumen F = {};

	// Organize Echoes
	valMap.clear();
	for( Body EB;  const auto [x, y, beat, radius, initLoop, deltaLoop, isSpecial] : N.echoes )
		if( const uint64_t ms = beatToMs(beat);  ms >= 637  &&  ms <= 1048106 )  [[likely]]  /* Arf4.h */
			EB = { .cdx = (int64_t)( FIX(x,8) + (x-8) * 16 ),   .radius = (uint64_t)( 0.5 + radius * 4 ),
				   .cdy = (int64_t)( FIX(y,4) + (y-4) * 16 ), .initLoop = (uint64_t)( 0.5 + initLoop * 64 ),
				   .deltaLoop = (int64_t)( FIX(deltaLoop,0) + deltaLoop * 8 ),  .ms = ms },
			valMap[EB.val].val |= isSpecial;   // 1048106: 1048576-470
		else return lua_pushboolean(L, false),
					lua_pushfstring(L, NEMESIS_TIME_OOR, ECHO), 2;

	const size_t echoSize = valMap.size();
	if( echoSize > 131072 )   /* Inout */
		return lua_pushboolean(L, false),
			   lua_pushfstring(L, NEMESIS_SLE, "Echoes", LI 131072), 2;
	/**/F.echoes.reserve( echoSize );
	for( Body e;  const auto [val, s] : valMap )
		F.echoes.push_back(( e.val = val,  e.status = s.val,  e ));
	std::ranges::sort( F.echoes, [](const Body a, const Body b) {
		if( auto amsyx = a.val << 27,  bmsyx = b.val << 27;  amsyx != bmsyx )		return amsyx < bmsyx;
		else																		return a.val < b.val;
	} );

	// Organize Times of Wishes, Add Hints into `F.hints`
	valMap.clear();
	for( auto& w : N.wishes ) {
		if( w.nodes.size() > 4096 )  [[unlikely]]  /* Arf4.h */
			return lua_pushboolean(L, false),
				   lua_pushfstring(L, NEMESIS_SLE, "a Wish's Nodes", LI 4096), 2;
		for( auto& n : w.nodes)
			if( n.beat = beatToMs( n.beat ),  n.beat > 1048575 )  [[unlikely]]  /* Arf4.h */
				return lua_pushboolean(L, false),
					   lua_pushfstring(L, NEMESIS_TIME_OOR, "Wish Node"), 2;
		const auto N0 = w.nodes.front(), NL = w.nodes.back();

		for( auto [beat, isSpecial] : w.manualHints )
			if( Body HB;  beat = beatToMs(beat),  beat >= 510  &&  beat > N0.beat  &&  beat <= NL.beat )
				wishCacheT(w, beat),  HB = { .ms = (uint64_t)beat,
											.cdx = (int64_t)(FIX(w.wX,8) + (w.wX-8) * 16),
											.cdy = (int64_t)(FIX(w.wY,4) + (w.wY-4) * 16) },
				valMap[HB.val].val |= isSpecial;

		for( auto& c : w.wishChilds )   // Use valueMap to deduplicate & sort childs later
			if( Body HB;  (c.beat = beatToMs(c.beat)) >= 510  &&  c.beat > N0.beat  &&  c.beat <= NL.beat )
				wishCacheT(w, c.beat),  HB = { .ms = (uint64_t)c.beat,
											  .cdx = (int64_t)(FIX(w.wX,8) + (w.wX-8) * 16),
											  .cdy = (int64_t)(FIX(w.wY,4) + (w.wY-4) * 16) },
				valMap.insert({ HB.val, {} });
		std::erase_if( w.wishChilds, [N0](const N4::Child& c) {  return c.beat <= N0.beat;  } );
	}

	const size_t hintSize = valMap.size();
	if( hintSize > 32767 )   /* Inout */
		return lua_pushboolean(L, false),
			   lua_pushfstring(L, NEMESIS_SLE, "Hints", LI 32767), 2;
	/**/F.hints.reserve( hintSize );
	for( Body h;  const auto [val, s] : valMap )
		F.hints.push_back(( h.val = val,  h.status = s.val,  h ));

	/* Generate hIdx & eIdx, Count scored objects
	 * Metadata: before, objectCount, hgoRequired, egoRequired
	 */
	if( hintSize )
		F.objectCount = hintSize,
		F.before = F.hints.back().ms + 470;
	if( uint32_t efinalMs;  echoSize )
		efinalMs = F.echoes.back().ms + 470,
		F.before < efinalMs ? (F.before = efinalMs) : 0;
	F.idx.resize( (F.before >> 10) + 1,  { .hSince = hintSize, .eSince = echoSize } );

	valMap.clear();
	for( uint32_t i = 0;  i < hintSize;  ++i )
		for( int ET = ( F.sHit < 31 ? (F.sHit += F.hints[i].status) : (F.hints[i].status = NJUDGED),
						F.hints[i].ms + 470 ),  LG = ET >> 10,  GRP = (ET - 980) >> 10;  GRP <= LG;  ++GRP )
			if( valMap.contains(GRP) )  [[likely]]			valMap[GRP].am += 1;
			else											valMap[GRP] = { .am = 1, .bm = i };
	for( const auto [group, cnt_since] : valMap )
		if( cnt_since.am > 511 )  [[unlikely]]  /* Arf4.h */
			return lua_pushboolean(L, false),
				   lua_pushfstring(L, NEMESIS_SLE, "Hints within 1024ms", LI 511), 2;
		else if( F.idx[group].hSince = cnt_since.bm,  cnt_since.am > F.hgoRequired )
			F.hgoRequired = cnt_since.am;

	valMap.clear();
	for( uint32_t i = 0;  i < echoSize;  ++i )
		if( const Body e = F.echoes[i];  e.status  &&  F.objectCount++ == 65535 ) [[unlikely]] /* Arf4.h */
			return lua_pushboolean(L, false),
				   lua_pushfstring(L, NEMESIS_SLE, "Objects", LI 65535), 2;
		else for( int ET = e.ms - (e.radius ? 75 * fmax(e.radius, 24) : 637.0),  LG = (e.ms + 470) >> 10,
					 GRP = (ET < 0 ? 0 : ET) >> 10;  GRP <= LG;  ++GRP )   // Lowest DBPM 50
			if( valMap.contains(GRP) )  [[likely]]			valMap[GRP].am += 1;
			else											valMap[GRP] = { .am = 1, .bm = i };
	for( const auto [group, cnt_since] : valMap )
		if( cnt_since.am > 511 )  [[unlikely]]  /* Arf4.h */
			return lua_pushboolean(L, false),
				   lua_pushfstring(L, NEMESIS_SLE, "Echoes within 1024ms", LI 511), 2;
		else if( F.idx[group].eSince = cnt_since.bm,  cnt_since.am > F.egoRequired )
			F.egoRequired = cnt_since.am;

	/* Flatten Nodes & WishChilds
	 * Metadata: before
	 */
	std::ranges::sort( N.wishes, [](auto& a, auto& b) {  return a.nodes[0].beat < b.nodes[0].beat;  } );
	F.wishChilds.reserve(32767),	idxProto.clear(),
	F.nodes.reserve(32767),			idxProto.resize(512);

	for( auto& w : N.wishes ) {
		Wish wView = { .nSince = F.nodes.size(),  .isSpecial = w.isSpecial,  .cIndex = 1 };

		// Organize Nodes
		if( const auto wNs = w.nodes.size();  wView.nSince + wNs + (wNs>2) > 262144 )   /* Inout */
			return lua_pushboolean(L, false),
				   lua_pushfstring(L, NEMESIS_SLE, "Wish Nodes", LI 262144), 2;
		for( const auto [x, y, beat, radius, degree, ease] : w.nodes )
			F.nodes.push_back({ .cdx = (int64_t)( FIX(x,8) + (x-8) * 16 ),   .ms = (uint64_t)beat,
								.cdy = (int64_t)( FIX(y,4) + (y-4) * 16 ),	 .ease = ease,
								.deg = (int64_t)( FIX(degree,0) + degree),
								.radius = (uint64_t)( 0.5 + radius * 4 ) });
		wView.nType = w.nodes.size() > 2 ? ( F.nodes.push_back({ }), 1 ) : 0;

		// Organize WishChilds
		valMap.clear();
		for(Child c;  const auto& nC : w.wishChilds)
			c = { .ms = (uint64_t)nC.beat,  .radius = (uint64_t)( 0.5 + nC.radius * 4 ),
										  .initLoop = (uint64_t)( 0.5 + nC.initLoop * 64 ),
										 .deltaLoop = (int64_t)( FIX(nC.deltaLoop,0) + nC.deltaLoop * 8 ) },
			valMap[c.val] = {};
		if( const size_t childCnt = valMap.size();  childCnt > 8192 ) [[unlikely]]   /* Arf4.h */
			return lua_pushboolean(L, false),
				   lua_pushfstring(L, NEMESIS_SLE, "a Wish's WishChilds", LI 8192), 2;
		else if( F.wishChilds.size() + childCnt + !!childCnt > 131072 ) [[unlikely]]   /* Inout */
			return lua_pushboolean(L, false),
				   lua_pushfstring(L, NEMESIS_SLE, "WishChilds", LI 131072), 2;
		else if( childCnt ) [[likely]] {
			for(const auto [cVal, _]  :  wView.cSince = F.wishChilds.size(), valMap)
				F.wishChilds.push_back({ .val = cVal });
			doubleMap.clear();
				for( constexpr double LOWEST_SPEED_RCP = 15000 / 50;  const auto& c : w.wishChilds )
					/* From */  doubleMap[ fmax(0, c.beat - fmax(c.radius, 6) * LOWEST_SPEED_RCP) ] += 1,
					/*  To  */  doubleMap[ c.beat ] -= 1;
				for( int16_t currentStep = 1 /* the Wish itself */ ;  const auto [_, stepIncr] : doubleMap )
					if( (currentStep += stepIncr) > wView.cIndex )
						wView.cIndex = currentStep;   // Borrowed to be wgoRequired for this Wish
			wView.cType = 1,  wView.withCs = w.withCs,  F.wishChilds.push_back({ });
		}

		/* Push this Wish into idxProto
		 * Update F.before
		 */
		const uint32_t lastMs = w.nodes.back().beat;
		for( uint32_t e = lastMs >> 11,  i = (uint32_t)w.nodes.front().beat >> 11;  i <= e;  ++i )
			idxProto[i].push_back( wView );
		(F.before < lastMs) ? (F.before = lastMs) : 0;
	}

	/* Flatten Wishes
	 * Metadata: wgoRequired
	 */
	const int16_t wiSize = (F.before >> 11) + 1;
		idxProto.resize( wiSize );
	for( int16_t iSize = F.idx.size();  iSize < wiSize;  ++iSize )
		F.idx.push_back({ .hSince = hintSize,  .eSince = echoSize });
	F.wishes.reserve(16384);   // Seems Okay

	for( uint16_t i = 0;  auto& group : idxProto )
		if( uint16_t groupWgoUsed = 0;  F.idx[i++].wSince = F.wishes.size(),  group.size() ) {
			for( auto wish : group )
				groupWgoUsed += wish.cIndex,  wish.cIndex = 0 /* End of the borrowing */,
				F.wishes.push_back( wish );
			if( F.wishes.size() > 16777216 )   /* Inout */
				return lua_pushboolean(L, false),
					   lua_pushfstring(L, NEMESIS_SLE, "Wishes", LI 16777216), 2;
			if( groupWgoUsed > 1023 )   /* Arf4.h */
				return lua_pushboolean(L, false),
					   lua_pushfstring(L, NEMESIS_SLE, WISHES_2048, LI 1023), 2;
			(F.wgoRequired < groupWgoUsed) ? (F.wgoRequired = groupWgoUsed) : 0;
		}

	return  Arf = { .val = F.val },					F.wishes.swap(Arf.wishes),
			lua_pushinteger(L, F.before),			F.nodes.swap(Arf.nodes),
			lua_pushinteger(L, F.objectCount),		F.hints.swap(Arf.hints),
			lua_pushinteger(L, F.wgoRequired),		F.wishChilds.swap(Arf.wishChilds),
			lua_pushinteger(L, F.hgoRequired),		F.echoes.swap(Arf.echoes),
			lua_pushinteger(L, F.egoRequired),		F.idx.swap(Arf.idx),
	5;
}
#endif