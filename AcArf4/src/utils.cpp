//  Arf4 Utils  //
#include <Arf4.h>
using namespace Ar;

/* Ease Utils */
#include <constants.h>
float Ar::Eased(const double ratio, const uint8_t type) noexcept {
	switch(type) {
	  default:	case STATIC:	return 0;
	[[likely]]	case LINEAR:	return ratio;
				case INSINE:	return inSine [(uint16_t)(ratio * 4096)];
				case OUTSINE:	return outSine[(uint16_t)(ratio * 4096)];
	}
}

Duo Ar::CosSin(Duo d) noexcept {   // Pass Degree into d.a
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

int Ar::Ease(lua_State* L) noexcept {
	/* Usage:
	 * local result = Arf4.Ease(from, type, to, ratio)
	 */
	const lua_Number from  = lua_tonumber(L, 1),
					 delta = lua_tonumber(L, 3) - from;
		  lua_Number ratio = lua_tonumber(L, 4);
	if		(ratio < 0)		  ratio = 0;
	else if (ratio > 1)		  ratio = 1;
	return lua_pushnumber(L,
		from + delta * Eased( ratio, lua_tointeger(L, 2) )
	), 1;
}

/* Misc Utils */
int Ar::SetCam(lua_State* L) noexcept {
	/* Usage:										 -- xScale / yScale: Mainly for Runtime Mirroring
	 * Arf4.SetCam(xscale, yscale, xdelta, camspd)   --			 xDelta: Mainly for Options Panel
	 */
	Arf.xScale = lua_tonumber(L, 1) * 14.0625;
	Arf.yScale = lua_tonumber(L, 2) * 14.0625;
	Arf.xDelta = lua_tonumber(L, 3);

	const lua_Number cSpeed = lua_tonumber(L, 4);
		Arf.cSpeed = cSpeed < 0 ? 0 : cSpeed > 1.25 ? 1.25 : cSpeed;
	return 0;
}

int Ar::SetSpeed(lua_State* L) noexcept {
	/* Usage:
	 * Arf4.SetSpeed(scale)
	 */
	const lua_Number  pSpeed = lua_tonumber(L, 1);
		PlayerSpeed = pSpeed < 0.5 ? 0.5 : pSpeed > 10 ? 10 : pSpeed;
	return 0;
}

int Ar::SetDaymode(lua_State* L) noexcept {
	lua_toboolean(L, 1) ?
		Arf.oTint = HintHit, Arf.aTint[0] = AnimHit:
		Arf.oTint = HintHr,  Arf.aTint[0] = AnimHr;
	return 0;
}

int Ar::SetJudgeZone(lua_State* L) noexcept {
	/* Usage:
	 * Arf4.SetJudgeZone(ms, is_any_x, is_any_y)   -- ms ∈ [1,100]
	 */
	const uint8_t zone = lua_tointeger(L, 1);
	Arf.judgeRange = zone > 99 ? 100 : zone < 1 ? 1 : zone;

	Arf.minDt = InputDelta - Arf.judgeRange;		Arf.minDt = Arf.minDt < -100 ? -100 : Arf.minDt ;
	Arf.maxDt = InputDelta + Arf.judgeRange;		Arf.maxDt = Arf.maxDt >  100 ?  100 : Arf.maxDt ;
	Arf.isAnyX = lua_toboolean(L, 2);
	Arf.isAnyY = lua_toboolean(L, 3);
	return 0;
}

int Ar::GetJudgeStat(lua_State* L) noexcept {
	/* Usage:
	 * local hint_hit, echo_hit, early, late, lost, special_hint_hit, object_count = Arf4.GetJudgeStat()
	 */
	return lua_pushinteger(L, Arf.hHit),  lua_pushinteger(L, Arf.eHit),  lua_pushinteger(L, Arf.early),
		   lua_pushinteger(L, Arf.late),  lua_pushinteger(L, Arf.lost),  lua_pushinteger(L, Arf.sHit),
		   lua_pushinteger(L, Arf.objectCount), 7;
}

int Ar::SetInputDelta(lua_State* L) noexcept {
	/* Usage:
	 * Arf4.SetInputDelta(ms)   -- [-63,63]
	 */
	const int8_t inputDeltaParam = lua_tointeger(L, 1);
	InputDelta = inputDeltaParam > 63 ? 63 : inputDeltaParam < -63 ? -63 : inputDeltaParam;

	Arf.minDt = InputDelta - Arf.judgeRange;		Arf.minDt = Arf.minDt < -100 ? -100 : Arf.minDt ;
	Arf.maxDt = InputDelta + Arf.judgeRange;		Arf.maxDt = Arf.maxDt >  100 ?  100 : Arf.maxDt ;
	return 0;
}