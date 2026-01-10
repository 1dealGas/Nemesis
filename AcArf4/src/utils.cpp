//  Arf4 Utils  //
#include <Arf4.h>
#include <map>

/* Ease Utils */
#include <constants.h>
float Ar::Eased(const double ratio, const uint8_t type) noexcept {
	switch(type) {
	  default:	case STATIC:	return 0;
	[[likely]]	case LINEAR:	return ratio;
				case INSINE:	return inSine [(uint16_t)(ratio * 1024)];
				case OUTSINE:	return outSine[(uint16_t)(ratio * 1024)];
	}
}

Ar::Duo Ar::CosSin(Duo d) noexcept {   // Pass Degree into d.a
	switch( d.as ) {
		case 0: default: {
			uint64_t deg16  = (d.ae+=4, d.a);		 const uint64_t deg16div1440 = deg16 / 1440;
					 deg16 -= deg16div1440 * 1440;
			switch( deg16div1440 & 0b11 ) {
				default:
				case 0: return d.b =  degreeSin[     deg16], d.a =  degreeSin[1440-deg16], d;   // 0~90
				case 1: return d.b =  degreeSin[1440-deg16], d.a = -degreeSin[     deg16], d;   // 90~180
				case 2: return d.b = -degreeSin[     deg16], d.a = -degreeSin[1440-deg16], d;   // 180~270
				case 3: return d.b = -degreeSin[1440-deg16], d.a =  degreeSin[     deg16], d;   // 270~360
			}
		}
		case 1: {   // d.f < 0, sin(-x) = -sin(x), cos(-x) = cos(x)
			uint64_t deg16  = (d.ae+=4, -d.a);		 const uint64_t deg16div1440 = deg16 / 1440;
					 deg16 -= deg16div1440 * 1440;
			switch( deg16div1440 & 0b11 ) {
				default:
				case 0: return d.b = -degreeSin[     deg16], d.a =  degreeSin[1440-deg16], d;
				case 1: return d.b = -degreeSin[1440-deg16], d.a = -degreeSin[     deg16], d;
				case 2: return d.b =  degreeSin[     deg16], d.a = -degreeSin[1440-deg16], d;
				case 3: return d.b =  degreeSin[1440-deg16], d.a =  degreeSin[     deg16], d;
			}
		}
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

static std::map<uint32_t, Ar::Duo> SM;
int Ar::NewSeries(lua_State* L) noexcept {
	/* Example:
	 * local S = Arf4.NewSeries {	-- When failed, a nil will be returned.
	 *     0, 1, LINEAR,			-- 0 ms, Val=1, Linear Ease
	 *     100, 2, LINEAR, ...  }
	 */
	for( uint32_t ms, inputLen = lua_objlen(L,1),  i = 1;  i < inputLen;  i += 3 )
		ms = ( lua_rawgeti(L, 1, i), lua_tointeger(L, 2) ),
		SM[ms] = { .v = (float)   ( lua_rawgeti(L, 1, i+1), lua_tonumber(L,3) ),	.ms = ms,
				  .es = (uint32_t)( lua_rawgeti(L, 1, i+2), lua_tonumber(L,4) ) },  lua_settop(L,1);
	if( const auto SZ = SM.size();  lua_settop(L,0),  SZ )
		for( auto& S = *new(lua_newuserdata(L, sizeof(std::vector<Duo>))) std::vector<Duo> {{.val = 1}};
			 auto  [_,K] : lua_getglobal(L, Ar::f4), lua_setmetatable(L,1), S.reserve(SZ), SM )
			S.push_back(K);
	return SM.clear(), 1;
}

int Ar::Ease(lua_State* L) noexcept {
	/* Usage:
	 * Arf4.Ease(t, series, results)
	 */
	for( size_t T = lua_tointeger(L,1), i = lua_objlen(L,2);  i;  lua_rawseti(L,3,i--), lua_settop(L,3) )
		if( auto& S = *(std::vector<Duo>*)(lua_rawgeti(L,2,i), lua_touserdata(L,4));  T < S[1].ms )
			lua_pushnumber(L, S[1].v);
		else if( Duo l, r = S.back();  T >= r.ms )
			lua_pushnumber(L, r.v);
		else if( auto& idx = S[0].val;  l = S[idx],  T < l.ms ) {
			do {--idx;}  while( l = S[idx],    T < l.ms );
								r = S[idx+1];  goto IP;			}
		else if( r = S[idx+1],  T >= r.ms )						{
			do {++idx;}  while( r = S[idx+1],  T >= r.ms );
								l = S[idx];    goto IP;			}
		else IP:
			lua_pushnumber( L, l.v + (r.v-l.v) * Eased( (double)(T-l.ms)/(r.ms-l.ms), l.es ) );
	return 0;
}

/* Misc Utils */
int Ar::SetCam(lua_State* L) noexcept {
	/* Usage:										 -- xScale / yScale: Mainly for Runtime Mirroring
	 * Arf4.SetCam(xscale, yscale, xdelta, camspd)   --			 xDelta: Mainly for Options Panel
	 */
	Arf.xScale = lua_tonumber(L, 1) * 7.03125;
	Arf.yScale = lua_tonumber(L, 2) * 7.03125;
	Arf.xDelta = lua_tonumber(L, 3);

	const lua_Number cSpeed = lua_tonumber(L, 4);
		Arf.cSpeed = cSpeed < 0  ?  0 : cSpeed;
	return 0;
}

#ifndef AR_BUILD_VIEWER
int Ar::SetOptions(lua_State* L) noexcept {
	/* Usage:
	 * Arf4.SetOptions(input_delta_ms, player_speed)
	 */
	InputDelta = lua_tointeger(L, 1);   // [-63, 63]
	 Arf.minDt = InputDelta - Arf.judgeRange;		Arf.minDt = Arf.minDt < -100 ? -100 : Arf.minDt;
	 Arf.maxDt = InputDelta + Arf.judgeRange;		Arf.maxDt = Arf.maxDt >  100 ?  100 : Arf.maxDt;
	PlayerSpeed = lua_tonumber(L, 1);   // [0.5, 10]
	return 0;
}

int Ar::SetJudgeZone(lua_State* L) noexcept {
	/* Usage:
	 * Arf4.SetJudgeZone(ms, is_any_x, is_any_y)   -- ms ∈ [0,100]
	 */
	Arf.judgeRange = lua_tointeger(L, 1);
	Arf.minDt = InputDelta - Arf.judgeRange;		Arf.minDt = Arf.minDt < -100 ? -100 : Arf.minDt ;
	Arf.maxDt = InputDelta + Arf.judgeRange;		Arf.maxDt = Arf.maxDt >  100 ?  100 : Arf.maxDt ;

	Arf.isAnyX = lua_toboolean(L, 2);
	Arf.isAnyY = lua_toboolean(L, 3);
	return 0;
}

int Ar::SetJudgeStat(lua_State* L) noexcept {
	/* Usage:
	 * Arf4.SetJudgeStat(hint_hit, echo_hit, early, late, lost, special_hint_hit)
	 */
	return Arf.hHit = lua_tointeger(L,1),  Arf.eHit = lua_tointeger(L,2),  Arf.early = lua_tointeger(L,3),
		   Arf.sHit = lua_tointeger(L,6),  Arf.lost = lua_tointeger(L,5),  Arf.late  = lua_tointeger(L,4), 0;
}

int Ar::GetJudgeStat(lua_State* L) noexcept {
	/* Usage:
	 * local hint_hit, echo_hit, early, late, lost, special_hint_hit = Arf4.GetJudgeStat()
	 */
	return lua_pushinteger(L, Arf.hHit),  lua_pushinteger(L, Arf.eHit),  lua_pushinteger(L, Arf.early),
		   lua_pushinteger(L, Arf.late),  lua_pushinteger(L, Arf.lost),  lua_pushinteger(L, Arf.sHit), 6;
}
#endif