//  Arf4 Utils  //
#include <Arf4.h>
#include <constants.h>

/* Ease Utils */
float Ar::Eased(const float ratio, const uint8_t type) noexcept {
	switch(type) {
	  default:	case STATIC:	return 0;
	[[likely]]	case LINEAR:	return ratio;
				case INSINE:	return 1 - xSin[ 1440 - (int)(ratio * 1440) ];
				case OUTSINE:	return 0 + xSin[		(int)(ratio * 1440) ];
	}
}

Ar::Duo Ar::CosSin(Duo d) noexcept {   // Pass Degree into d.a
	switch( uint32_t D16 = ( d.ae += 4, (d.as ? -d.a : d.a) ),	Qdt  = D16 / 1440;
																D16 -= Qdt * 1440,	Qdt & 0b11 ) {
		default:
		case 0: return d.b = xSin[	   D16], d.bs =  d.as, d.a =  xSin[1440-D16], d;   // 0~90
		case 1: return d.b = xSin[1440-D16], d.bs =  d.as, d.a = -xSin[		D16], d;   // 90~180
		case 2: return d.b = xSin[	   D16], d.bs = ~d.as, d.a = -xSin[1440-D16], d;   // 180~270
		case 3: return d.b = xSin[1440-D16], d.bs = ~d.as, d.a =  xSin[		D16], d;   // 270~360
	}
}

int Ar::GetCosSin(lua_State* L) noexcept {
	/* Usage:
	 * local cos, sin = Arf4.GetCosSin(degree)
	 */
	const Duo cosSin = CosSin({ (float)lua_tonumber(L,1) });
	return lua_pushnumber(L, cosSin.a),
		   lua_pushnumber(L, cosSin.b), 2;
}

int Ar::NewSeries(lua_State* L) noexcept {
	/* Example:
	 * local S = Arf4.NewSeries {	-- Internal Fn, No Error Checking
	 *     0, 1, LINEAR,			-- 0 ms, Val=1, Linear Ease
	 *     100, 2, LINEAR, ...  }
	 */
	const auto C = lua_objlen(L,1), sSize = (C+1) / 3;
	const auto S = (Duo*)lua_newuserdata(L, (sSize << 3) + 8);
	for( uint32_t n = 0, i = 1;  i < C;  i += 3 )
		S[++n] = {  .a  = (float)	( lua_rawgeti(L, 1, i+1), lua_tonumber (L,3) ),
					.ms = (uint32_t)( lua_rawgeti(L, 1, i  ), lua_tointeger(L,4) ),
					.es = (uint32_t)( lua_rawgeti(L, 1, i+2), lua_tointeger(L,5) )  },
		lua_settop(L,2);
	return (*S = S[sSize]), (S->es = 1);
}

int Ar::Ease(lua_State* L) noexcept {
	/* Usage:
	 * Arf4.Ease(t, series, results)
	 */
	for( size_t  T = lua_tointeger(L,1), i = lua_objlen(L,2);  i;  lua_rawseti(L,3,i--), lua_settop(L,3) )
		if( Duo *S = (Duo*)( lua_rawgeti(L,2,i), lua_touserdata(L,4) ),  l = S[1];  T < l.ms )
			lua_pushnumber(L, l.a);
		else if( Duo r = *S;  T >= r.ms )
			lua_pushnumber(L, r.a);
		else if( auto idx = r.es;	l = S[idx],  T < l.ms )		{
			do { --idx; }	 while( l = S[idx],  T < l.ms );
									r = S[idx+1];  goto CAL;	}
		else if( r = S[idx+1],  T >= r.ms )						{
			do { ++idx; }	 while( r = S[idx+1],  T >= r.ms );
									l = S[idx];    goto CAL;	}
		else CAL:   // Calculate the Eased Value
			S->es = idx,  lua_pushnumber( L, l.a + (r.a-l.a) * Eased( (float)(T-l.ms)/(r.ms-l.ms), l.es ) );
	return 0;
}

/* Misc Utils */
int Ar::SetCam(lua_State* L) noexcept {
	/* Usage:												 -- xScale / yScale: Mainly for Mirroring
	 * Arf4.SetCam(xscale, yscale, xdelta, cspeed, vstime)   --			 xDelta: Mainly for Options Panel
	 */
	Arf.xScale = lua_tonumber(L, 1) * 7.03125;		Arf.xDelta = lua_tonumber(L, 3);
	Arf.yScale = lua_tonumber(L, 2) * 7.03125;		Arf.cSpeed = lua_tonumber(L, 4);
	Arf.vsTime = lua_tonumber(L, 5);				Arf.cSpeed < 0 ? Arf.cSpeed = 0 : 0;
	return 0;
}

#ifndef AR_BUILD_VIEWER
int Ar::SetOptions(lua_State* L) noexcept {
	/* Usage:
	 * Arf4.SetOptions(input_delta_ms, player_speed)
	 */
	InputDelta = lua_tointeger(L, 1);   // [-63, 63]
	PlayerSpeed = lua_tonumber(L, 2) / 375;
	return 0;
}

int Ar::SetJudgeZone(lua_State* L) noexcept {
	/* Usage:
	 * Arf4.SetJudgeZone(ms, is_any_x, is_any_y)   -- ms ∈ [0,100]
	 */
	Arf.isAnyX = lua_toboolean(L, 2);				Arf.judgeZone = lua_tointeger(L, 1);
	Arf.isAnyY = lua_toboolean(L, 3);				Arf.judgeZone > 100  ?  Arf.judgeZone = 100 : 0;
	return 0;
}

int Ar::GetJudgeStat(lua_State* L) noexcept {
	/* Usage:
	 * local hit, early, late, lost, hint_hit, echo_hit, special_hit = Arf4.GetJudgeStat()
	 */
	return lua_pushinteger(L, Arf.hit),   lua_pushinteger(L, Arf.early),	lua_pushinteger(L, Arf.late),
		   lua_pushinteger(L, Arf.lost),  lua_pushboolean(L, Arf.hintHit),	lua_pushboolean(L, Arf.echoHit),
		   lua_pushinteger(L, Arf.sHit),  (Arf.hintHit = Arf.echoHit = 0),
	7;
}
#endif