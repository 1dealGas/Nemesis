// Nemesis, the Aerials Fumen Compiler. //
#ifndef AR_BUILD_VIEWER
#include <unordered_map>
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
		uint8_t				nIdx, isSpecial, compressChild;
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
	return thiz.toneBase + fmax(bar - thiz.bar, 0) * thiz.a / thiz.b;   // tone >= 0 guaranteed
}

static double toneToBar(const double tone) noexcept {   // Internal, tone >= 0 required
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
	return thiz.bar + (tone - thiz.toneBase) * thiz.b / thiz.a;
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

static double checkTime(lua_State* L, const int where) noexcept {   // Stack balanced; Won't pop
	if( double T;  lua_istable(L, where) )
		return N.sinceTone = barToTone(( lua_rawgeti(L, where, 1), lua_tonumber(L, -1) )),	lua_pop(L, 1),
			   T = ( lua_rawgeti(L, where, 2), lua_tonumber(L, -1) ),						lua_pop(L, 1),
			   barToBeat(toneToBar(  N.sinceTone + ( T<0 ? -T/16 : T )  ));
	else
		return T = lua_tonumber(L, where),
			   barToBeat(toneToBar(  N.sinceTone + ( T<0 ? -T/16 : T )  ));
}

static double checkTimeLocal(lua_State* L, const int where) noexcept {   // Stack balanced; Won't pop
	if( double S, T;  lua_istable(L, where) )
		return S = barToTone(( lua_rawgeti(L, where, 1), lua_tonumber(L, -1) )),			lua_pop(L, 1),
			   T = ( lua_rawgeti(L, where, 2), lua_tonumber(L, -1) ),						lua_pop(L, 1),
			   barToBeat(toneToBar(  S + ( T<0 ? -T/16 : T )  ));
	else
		return T = lua_tonumber(L, where),
			   barToBeat(toneToBar(  N.sinceTone + ( T<0 ? -T/16 : T )  ));
}

static N4::Point checkPointArg(lua_State* L, const int where) noexcept {   // Stack balanced; Won't pop
	if( lua_istable(L, where) ) {
		const auto r = ( lua_rawgeti(L, where, 1), lua_tonumber(L, -1) );					lua_pop(L, 1);
		const auto d = ( lua_rawgeti(L, where, 2), lua_tonumber(L, -1) );					lua_pop(L, 1);
		const auto e = ( lua_rawgeti(L, where, 3), lua_tonumber(L, -1) );					lua_pop(L, 1);
		return {
			.radius = fmin((uint8_t)(r * 4), 31) * 0.25,
			.degree = d < -1024 ? -1024 : d > 1023 ? 1023 : d,
			.ease = (uint8_t)(e > Arf4::OUTSINE  ?  Arf4::LINEAR : e)
		};
	}
	const auto e = lua_tonumber(L, where);
	return {.ease = (uint8_t)(e > Arf4::OUTSINE  ?  Arf4::LINEAR : e)};
}

static int wishGetActualX(lua_State* L) noexcept {
	N4::Wish* w;
	if( lua_isuserdata(L, 1) )
		w = (N4::Wish*)lua_touserdata(L, 1), wishCacheT( *w, checkTimeLocal(L, 2) );
	else
		w = (N4::Wish*)lua_touserdata(L, 2), wishCacheT( *w, checkTimeLocal(L, 1) );
	lua_pushnumber(L, w->wX);
	return 1;
}

static int wishGetActualY(lua_State* L) noexcept {
	N4::Wish* w;
	if( lua_isuserdata(L, 1) )
		w = (N4::Wish*)lua_touserdata(L, 1), wishCacheT( *w, checkTimeLocal(L, 2) );
	else
		w = (N4::Wish*)lua_touserdata(L, 2), wishCacheT( *w, checkTimeLocal(L, 1) );
	lua_pushnumber(L, w->wY);
	return 1;
}

static int wishGetInfo(lua_State* L) noexcept {
	const auto w = (N4::Wish*)lua_touserdata(L, 1);
	wishCacheT( *w, checkTimeLocal(L, 2) );
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


/* Script APIs */
static std::map<double, N4::Delta> bpmMap;
static std::map<double, N4::Tempo> tempoMap;
int Ar::NewBuild(lua_State* L) noexcept {
	/* Example:
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
	if(! lua_istable(L, 1) )
		return lua_pushboolean(L, false), 1;
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
		lua_pushcfunction(L, wishGetActualX),	lua_setfield(L, -2, "__add"),
		lua_pushcfunction(L, wishGetActualX),	lua_setfield(L, -2, "__mul"),
		lua_pushcfunction(L, wishGetActualY),	lua_setfield(L, -2, "__sub"),
		lua_pushcfunction(L, wishGetActualY),	lua_setfield(L, -2, "__div"),
		lua_pushcfunction(L, wishGetInfo),		lua_setfield(L, -2, "__call");
	if( luaL_newmetatable(L, "NEMESIS_HELPER") )
		lua_pushcfunction(L, wishGetActualX),	lua_setfield(L, -2, "__add"),
		lua_pushcfunction(L, wishGetActualX),	lua_setfield(L, -2, "__mul"),
		lua_pushcfunction(L, wishGetActualY),	lua_setfield(L, -2, "__sub"),
		lua_pushcfunction(L, wishGetActualY),	lua_setfield(L, -2, "__div"),
		lua_pushcfunction(L, wishGetInfo),		lua_setfield(L, -2, "__call"),
		lua_pushcfunction(L, freeHelper),		lua_setfield(L, -2, "__gc");
	return lua_pushboolean(L, true), 1;
}

static std::map<double, double> deltaMap;
int Ar::SetDelta(lua_State* L) noexcept {
	/* Example:
	 * Delta {
	 *     {0},			1,				-- Bar 0, Ratio: 1
	 *     {2, 1/32},	-1,				-- Bar 2, then 1/32 Tone, Ratio: -1
	 *     {2, 1},		0.9,			-- Bar 2, then 1/16 Tone, Ratio: 0.9
	 *     15,			1,				-- Bar 2(Cached), then 15/16 Tone, Ratio: 1
	 *     ···
	 * }
	 */
	if(! lua_istable(L, 1) )
		return lua_pushboolean(L, false), 1;
	deltaMap.clear();

	const size_t inputLen = lua_objlen(L, 1);
	for( size_t i = 1;  i < inputLen;  i += 2 ) {
		const double ms = beatToMs(( lua_rawgeti(L, 1, i), checkTime(L, -1) )),
					 ratio = ( lua_rawgeti(L, 1, i+1), lua_tonumber(L, -1) );
		deltaMap[ms] = ratio;
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

				if( thisNode.base < 0  ||  thisNode.base > 1048575 * (8 - 1.0/1024) ) {
					N.deltas.clear();  N.deltas.push_back({ 0, 1 });
					return lua_pushboolean(L, false), 1;
				}
			}
			return lua_pushboolean(L, true), 1;
	}
}

static std::map<double, N4::Point> nodeMap;
int Ar::NewWish(lua_State* L) noexcept {
	/* Example:
	 * local myWish = Wish {			-- When failed, a nil will be returned.
	 *     Special = true,				-- false by default
	 *     CompressChild = true,		-- false by default
	 *     {1}, 4, 3, LINEAR,			-- Bar 1, X=4, Y=3, Linear Ease
	 *
	 *     -- Add Radius(5 here) & Degree(0 here) like this
	 *     -12, oldWish + (-12), oldWish - (-12), {5, 0, LINEAR},
	 *     ···
	 * }
	 */
	if(! lua_istable(L, 1) )
		return lua_pushnil(L), 1;
	N4::Wish W = {
		.isSpecial = (uint8_t)( lua_getfield(L, 1, "Special"), lua_toboolean(L,-1) ),
		.compressChild = (uint8_t)( lua_getfield(L, 1, "CompressChild"), lua_toboolean(L,-1) )
	};
	lua_pop(L, 2);

	nodeMap.clear();
	const size_t inputLen = lua_objlen(L, 1);
	for( size_t i = 1;  i < inputLen;  i += 4 ) {
		const double beat = ( lua_rawgeti(L, 1, i), checkTime(L, -1) ),
					 x = ( lua_rawgeti(L, 1, i+1), lua_tonumber(L, -1) ),
					 y = ( lua_rawgeti(L, 1, i+2), lua_tonumber(L, -1) );
		auto point = ( lua_rawgeti(L, 1, i+3), checkPointArg(L, -1) );
			 point.x = x < -55.875 ? -55.875 : x > 71.875 ? 71.875 : x;
			 point.y = y < -27.875 ? -27.875 : y > 35.875 ? 35.875 : y;
			 point.beat = beat;
		nodeMap[( nodeMap.contains(beat) ? nextDouble(beat) : beat )] = point;
		lua_pop(L, 4);
	}
	if( nodeMap.empty()  ||  nodeMap.cbegin()->second.beat == nodeMap.crbegin()->second.beat )
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
	 * local myHelperOrNil = Helper {	-- For getX/getY usages only.
	 *     {1}, 4, 3, LINEAR,			-- Bar 1, X=4, Y=3, Linear Ease
	 *     -12, 8, 9, LINEAR,			-- Bar 1 then 12/16 Tone, X=8, Y=9, Linear Ease
	 *     ···
	 * }
	 */
	if(! lua_istable(L, 1) )
		return lua_pushnil(L), 1;
	const auto  W = new(lua_newuserdata( L, sizeof(N4::Wish) )) N4::Wish;
		  auto& nodes = W -> nodes;
	nodeMap.clear();

	const size_t inputLen = lua_objlen(L, 1);
	for( size_t i = 1;  i < inputLen;  i += 4 ) {
		const double beat = ( lua_rawgeti(L, 1, i), checkTime(L, -1) ),
					 x = ( lua_rawgeti(L, 1, i+1), lua_tonumber(L, -1) ),
					 y = ( lua_rawgeti(L, 1, i+2), lua_tonumber(L, -1) );
		auto point = ( lua_rawgeti(L, 1, i+3), checkPointArg(L, -1) );
			 point.x = x < -55.875 ? -55.875 : x > 71.875 ? 71.875 : x;
			 point.y = y < -27.875 ? -27.875 : y > 35.875 ? 35.875 : y;
			 point.beat = beat;
		nodeMap[( nodeMap.contains(beat) ? nextDouble(beat) : beat )] = point;
		lua_pop(L, 4);
	}
	if( nodeMap.empty()  ||  nodeMap.cbegin()->second.beat == nodeMap.crbegin()->second.beat )
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
	 *     Wish = nil,					-- The last Wish of the Fumen by default
	 *     Radius = 7.0,				-- 7.0 by Default
	 *     Special = false,				-- Try to generate a special Hint if true, false by default
	 *     InitLoop = 0.25,				-- 0.25 by default
	 *     DeltaLoop = 1.25,			-- 0 by default
	 *     {1, 1}, 2, 3, 4, ···			-- Times
	 * }
	 */
	if( N.wishes.empty()  ||  lua_istable(L, 1) == 0 )
		return lua_pushboolean(L, false), 1;

	auto& W = N.wishes.back();
	if( lua_getfield(L, 1, "Wish"), lua_getmetatable(L, -1), luaL_getmetatable(L, "NEMESIS_WISH"),
		lua_equal(L, -1, -2) )
		W = *(N4::Wish*)lua_touserdata(L, 2);
	double radius = ( lua_getfield(L, 1, "Radius"), lua_tonumber(L, -1) );
		   radius = radius ? (radius > 7.75 ? 7.75 : radius) : 7;
	double initLoop = ( lua_getfield(L, 1, "InitLoop"), lua_tonumber(L, -1) );
		   initLoop = initLoop ? (initLoop < 0 ? 0 : initLoop > 1 ? 1 : initLoop) : 0.25;
	double deltaLoop = ( lua_getfield(L, 1, "DeltaLoop"), lua_tonumber(L, -1) );
		   deltaLoop = deltaLoop < -1.875 ? -1.875 : deltaLoop > 1.875 ? 1.875 : deltaLoop;
	const bool hintSpecial = ( lua_getfield(L, 1, "Special"), lua_toboolean(L, -1) );
	lua_pop(L, 4);

	const double minBeat = W.nodes.front().beat;
	const size_t inputLen = lua_objlen(L, 1);			W.wishChilds.reserve( inputLen );
	for( size_t i = 1;  i < inputLen;  ++i )
		if( const double beat = (lua_rawgeti(L, 1, i), checkTime(L, -1));  lua_pop(L, 1),  beat > minBeat )
			W.wishChilds.push_back({beat, radius, initLoop, deltaLoop, hintSpecial});
	return lua_pushboolean(L, true), 1;
}

int Ar::NewHint(lua_State* L) noexcept {
	/* Usage:
	 * Hint {
	 *     Wish = myWish,				-- The last Wish of the Fumen by default
	 *     Special = false,				-- False by default
	 *     {1}, 1, 2, 3, 4, ···			-- Times
	 * }
	 */
	if( N.wishes.empty()  ||  lua_istable(L, 1) == 0 )
		return lua_pushboolean(L, false), 1;

	auto& W = N.wishes.back();
	if( lua_getfield(L, 1, "Wish"), lua_getmetatable(L, -1), luaL_getmetatable(L, "NEMESIS_WISH"),
		lua_equal(L, -1, -2) )
		W = *(N4::Wish*)lua_touserdata(L, 2);
	const bool hintSpecial = ( lua_getfield(L, 1, "Special"), lua_toboolean(L, -1) );
	lua_pop(L, 1);

	const double minBeat = W.nodes.front().beat,
				 maxBeat = W.nodes.back().beat;
	const size_t inputLen = lua_objlen(L, 1);			W.manualHints.reserve( inputLen );
	for( size_t i = 1;  i < inputLen;  ++i )
		if( const double beat = ( lua_rawgeti(L, 1, i), checkTime(L, -1) );  lua_pop(L, 1),
			beat > minBeat  &&  beat <= maxBeat )
			W.manualHints.push_back({beat, hintSpecial});
	return lua_pushboolean(L, true), 1;
}

int Ar::NewEcho(lua_State* L) noexcept {
	/* Usage:
	 * Echo {
	 *     Radius = 7.0,				-- 0 by Default
	 *     Special = false,				-- Scored if true, false by default
	 *     InitLoop = 0.25,				-- 0.25 by default, ignored if Radius is 0
	 *     DeltaLoop = 1.25,			-- 0 by default, ignored if Radius is 0
	 *     {1}, 8, 0.5,					-- T1, X1, Y1
	 *     12, 8, 0.5,					-- T2, X2, Y2
	 *     ···
	 * }
	 */
	if(! lua_istable(L, 1) )
		return lua_pushboolean(L, false), 1;
	const bool isSpecial = (lua_getfield(L, 1, "Special"), lua_toboolean(L, -1));
		   double radius = (lua_getfield(L, 1, "Radius"), lua_tonumber(L, -1)), initLoop = 0, deltaLoop = 0;
				  radius = radius ? (radius > 7.75 ? 7.75 : radius) : 7;
	lua_pop(L, 2);

	if( radius )
		initLoop = ( lua_getfield(L, 1, "InitLoop"), lua_tonumber(L, -1) ),
		initLoop = initLoop ? (initLoop < 0 ? 0 : initLoop > 1 ? 1 : initLoop) : 0.25,
		deltaLoop = ( lua_getfield(L, 1, "DeltaLoop"), lua_tonumber(L, -1) ),
		deltaLoop = deltaLoop < -1.875 ? -1.875 : deltaLoop > 1.875 ? 1.875 : deltaLoop,
	/**/lua_pop(L, 2);

	const size_t inputLen = lua_objlen(L, 1);			N.echoes.reserve(inputLen);
	for( size_t i = 1;  i < inputLen;  i += 3 ) {
		const double beat = ( lua_rawgeti(L, 1, i), checkTime(L, -1) ),
					 x = ( lua_rawgeti(L, 1, i+1), lua_tonumber(L, -1) ),
					 y = ( lua_rawgeti(L, 1, i+2), lua_tonumber(L, -1) );
		N.echoes.push_back({
			.x = x < -55.875 ? -55.875 : x > 71.875 ? 71.875 : x,
			.y = y < -27.875 ? -27.875 : y > 35.875 ? 35.875 : y,
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
	N.verseWidx = N.wishes.size();
	N.verseEidx = N.echoes.size();
	return 0;
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
	if( double S;  lua_istable(L, 1) )   // [-2] sinceBar  [-1] withTone
		lua_rawgeti(L, 1, 1),  lua_rawgeti(L, 1, 2),  S = barToTone( lua_tonumber(L, -2) ),
		LT = lua_tonumber(L, -1),  LT = barToBeat(toneToBar(  S + ( LT<0 ? -LT/16 : LT )  ));
	else
		LT = lua_tonumber(L, +1),  LT = barToBeat(toneToBar(  N.sinceTone + ( LT<0 ? -LT/16 : LT )  ));

	double RT;
	if( double S;  lua_istable(L, 2) )   // [-2] sinceBar  [-1] withTone
		lua_rawgeti(L, 2, 1),  lua_rawgeti(L, 2, 2),  S = barToTone( lua_tonumber(L, -2) ),
		RT = lua_tonumber(L, -1),  RT = barToBeat(toneToBar(  S + ( RT<0 ? -RT/16 : RT )  ));
	else
		RT = lua_tonumber(L, +2),  RT = barToBeat(toneToBar(  N.sinceTone + ( RT<0 ? -RT/16 : RT )  ));

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
#include <utility>
static std::map<uint64_t, uint8_t> valueMap;
static std::unordered_map<uint64_t, uint8_t> echoMap;
int Ar::OrganizeArf(lua_State* L) noexcept {
	/* Usage:
	 * local before_or_false, objcnt, wgo_required, hgo_required, ego_required = Arf4.OrganizeArf()
	 */
	Fumen F = { .isAuto = true };

	// Organize Deltas
	F.deltas.reserve( N.deltas.size() + 1 ), F.deltas.push_back({ .val = 0 });
	for( const auto& d : N.deltas ) {
		if( d.init > 1048575 )
			return lua_pushboolean(L, false), 1;
		F.deltas.push_back({
			.val = (uint64_t)d.init >> 2,
			.absV = (uint64_t)( fmin( abs(d.value), 8 - 1.0/1024 ) * 1024 ),
			.base = (uint64_t)( d.base * 1024 )
		});
	}

	// Organize Echoes
	echoMap.clear();
	for( const auto [x, y, beat, radius, initLoop, deltaLoop, isSpecial] : N.echoes )
		if( const uint64_t ms = beatToMs(beat);  ms < 510  ||  ms > 1048575 - 470 )
			return lua_pushboolean(L, false), 1;
		else if( const auto baseEcho = Echo { .cdx = (int64_t)( (x - 8) * 8 ),
											  .cdy = (int64_t)( (y - 4) * 8 ),  .ms = ms,
											  .radius = (uint64_t)( radius * 4 ),
											  .initLoop = (uint64_t)( initLoop * 64 ),
											  .deltaLoop = (int64_t)( deltaLoop * 8 ) };
		echoMap[baseEcho.val] == false )   // Insertion and Value Checking, in one sentence
			echoMap[baseEcho.val] = isSpecial;

	F.echoes.reserve( echoMap.size() );
	for( const auto [val, isSpecial] : echoMap )
		if( Echo e = { .val = val };  true )
			F.echoes.push_back(( e.status = isSpecial, e ));
	std::ranges::sort( F.echoes, [](const Echo a, const Echo b) { return  a.val << 27  <  b.val << 27; } );

	// Organize Times of Wishes, Add Hints into `F.hints`
	valueMap.clear();
	for( auto& w : N.wishes ) {
		for( auto& n : w.nodes )
			if( (n.beat = beatToMs( n.beat )) > 1048575 )
				return lua_pushboolean(L, false), 1;
		for( auto& c : w.wishChilds ) {   // Use valueMap to deduplicate & sort childs later
			if( (c.beat = beatToMs( c.beat )) < w.nodes.back().beat  &&  c.beat >= 510 ) {
				wishCacheT(w, c.beat);
				if( auto baseHint = Hint { .cdx = (int64_t)( (w.wX - 8) * 8 ),
										   .cdy = (int64_t)( (w.wY - 4) * 8 ),
										   .ms = (uint64_t)c.beat };
				valueMap[baseHint.val] == false )
					valueMap[baseHint.val] = c.hintSpecial;
			}
			c.beat = msToDt(c.beat);
		}
		for( auto [beat, isSpecial] : w.manualHints )
			if( beat = beatToMs(beat), beat >= 510 ) {
				wishCacheT(w, beat);
				if( auto baseHint = Hint { .cdx = (int64_t)( (w.wX - 8) * 8 ),
										   .cdy = (int64_t)( (w.wY - 4) * 8 ),
										   .ms = (uint64_t)beat };
				valueMap[baseHint.val] == false )
					valueMap[baseHint.val] = isSpecial;
			}
	}

	F.hints.reserve( valueMap.size() );
	for( const auto [val, isSpecial] : valueMap )
		if( Hint h = { .val = val };  true )
			F.hints.push_back(( h.status = isSpecial, h ));
	std::ranges::sort( N.wishes, [](const auto& a, const auto& b) {
		return a.nodes.front().beat < b.nodes.front().beat;
	});

	// Generate hIdx & eIdx, Count scored objects
	// Metadata: before, objectCount, hgoRequired, egoRequired

	// Flatten Wishes, Generate wIdx
	// Metadata: before, wgoRequired

	return
		lua_pushinteger(L, F.before),			lua_pushinteger(L, F.objectCount),
		lua_pushinteger(L, F.wgoRequired),		lua_pushinteger(L, F.hgoRequired),
		lua_pushinteger(L, F.egoRequired),		Arf = std::move(F),
	5;
}
#endif