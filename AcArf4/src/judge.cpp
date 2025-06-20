//  Arf4 Judge  //
#include <Arf4.h>
#ifndef AR_BUILD_VIEWER
using namespace Ar;

/* Judge Behaviors:
 * [1] Tap Behavior
 * 	   Earliest Path / Anmitsu Path
 * [2] Echo Behavior
 *     Catch Path / Drag Path
 * [3] Lost Behavior
 */
static constexpr uint16_t OBJECT_SIZE = 456,
							HALF_SIZE = OBJECT_SIZE >> 1;
static bool hasTouchNear(const int16_t cdx, const int16_t cdy, const Duo validTouches[]) noexcept {
	switch( uint8_t whichTouch = 0;  Arf.isAnyX | Arf.isAnyY<<1 ) {
		case 3:
			return true;
		case 2: /* isAnyY */ {
			const float l = (900.0f - HALF_SIZE) + cdx * Arf.xScale + Arf.xDelta, r = l + OBJECT_SIZE;
			while(~ validTouches[whichTouch].val )   // Using {.a=NaN, .b=NaN} as the ending identifier
				if( const float touchX = validTouches[whichTouch].a;	  whichTouch++,
					touchX >= l  &&  touchX <= r )
					return true;
			return false;
		}
		case 1: /* isAnyX */ {
			const float d = (540.0f - HALF_SIZE) + cdy * Arf.yScale, u = d + OBJECT_SIZE;
			while(~ validTouches[whichTouch].val )
				if( const float touchY = validTouches[whichTouch].b;	  whichTouch++,
					touchY >= d  &&  touchY <= u )
					return true;
			return false;
		}
		[[likely]] default: {
			const float l = (900.0f - HALF_SIZE) + cdx * Arf.xScale + Arf.xDelta,	r = l + OBJECT_SIZE;
			const float d = (540.0f - HALF_SIZE) + cdy * Arf.yScale,				u = d + OBJECT_SIZE;
			while(~ validTouches[whichTouch].val )
				if( const Duo touch = validTouches[whichTouch];			  whichTouch++,
					touch.a >= l  &&  touch.a <= r  &&  touch.b >= d  &&  touch.b <= u )
					return true;
			return false;
		}
	}
}

#include <vector>
static std::vector<Duo> blockedPos;
static bool testAnmitsuSafety(const int16_t cdx, const int16_t cdy, const bool isScored) noexcept {
	const float x = 900.0f + cdx * Arf.xScale + Arf.xDelta,  y = 540.0f + cdy * Arf.yScale,
				l = x-OBJECT_SIZE,  r = x+OBJECT_SIZE,  d = y-OBJECT_SIZE,  u = y+OBJECT_SIZE;
	switch( Arf.isAnyX | Arf.isAnyY<<1 ) {
		case 3:
			return false;
		case 2: /* isAnyY */
			for(const auto i : blockedPos)
				if( i.a > l  &&  i.a < r )
					return false;
			break;
		case 1: /* isAnyX */
			for(const auto i : blockedPos)
				if( i.b > d  &&  i.b < u )
					return false;
			break;
		[[likely]] default:
			for(const auto i : blockedPos)
				if( i.a > l  &&  i.a < r  &&  i.b > d  &&  i.b < u )
					return false;
	}
	if( isScored )   // Push "Safe when Anmitsu" Objects
		blockedPos.push_back({ .a = x, .b = y });
	return true;
}

static Hint scanHint(Hint hint, const Duo validTouches[]) noexcept {
	switch( hint.status ) {
		case NJUDGED:		case NJUDGED_LIT:
		case SPECIAL:		case SPECIAL_LIT:
			hint.status = ( hasTouchNear(hint.cdx, hint.cdy, validTouches) << 1 ) + (hint.status & SPECIAL);
			return hint;
		case HIT_LIT:
		case EARLY_LIT:		case LATE_LIT:
			hint.status -=! hasTouchNear(hint.cdx, hint.cdy, validTouches) << 1;
		default:
			return hint;
	}
}

static Echo scanEcho(Echo echo, const int32_t deltaMs, const Duo validTouches[]) noexcept {
	if( echo.status & HIT )
		echo.status & 2  ?  echo.status -=! hasTouchNear(echo.cdx, echo.cdy, validTouches) << 1 : 0;
	else if( deltaMs < 101 )
		if( echo.status & NJUDGED_LIT )									 /* [2] Echo Behavior · Drag Path */
			hasTouchNear(echo.cdx, echo.cdy, validTouches) ? 0  :  (deltaMs > -88) ?
				Arf.eHit += echo.status & SPECIAL,  echo.status += 2,  echo.deltaMs = deltaMs:
				echo.status &= SPECIAL;
		else
			echo.status += hasTouchNear(echo.cdx, echo.cdy, validTouches) << 1;
	return echo;
}

#include <span>
static void judgeArfInternal(const Duo validTouches[], const bool anyPressed, const bool anyRel) noexcept {
	if( anyRel )
		blockedPos.clear();
	if( uint32_t G = Arf.msTime >> 10, minJudgedMs = NULL;  anyPressed ) {
		for(const Info ei = Arf.eIdx[G];  Echo& E : std::span(Arf.echoes).subspan(ei.f, ei.c))
			if( const int32_t DM = Arf.msTime - E.ms;  DM < -370 )		break;
			else if									 ( DM > +470 )		{}			   /* [1] Tap Behavior*/
			else if( (E = scanEcho(E, DM, validTouches)).status >> 1 == 1  &&  DM > -101  &&  DM < 101 ) {
				const bool safeToAnmitsu = testAnmitsuSafety(E.cdx, E.cdy, E.status & SPECIAL);
				if( !minJudgedMs )
					minJudgedMs = E.ms;
				else if( minJudgedMs != E.ms )				  // Consider if maxDt < 0
					if( !safeToAnmitsu || DM < Arf.minDt || DM > Arf.maxDt )
						continue;
				Arf.eHit += E.status & SPECIAL,  E.status += HIT;
				E.deltaMs = DM;
			}
		for(const Info hi = Arf.hIdx[G];  Hint& H : std::span(Arf.hints).subspan(hi.f, hi.c))
			if( const int32_t DM = Arf.msTime - H.ms;  DM < -370 )		break;
			else if									 ( DM > +470 )		{}
			else if( (H = scanHint(H, validTouches)).status >> 1 == 1  &&  DM > -101  &&  DM < 101 ) {
				const bool safeToAnmitsu = testAnmitsuSafety(H.cdx, H.cdy, true);
				if( !minJudgedMs  ||  minJudgedMs >= H.ms )
					minJudgedMs = H.ms;
				else if( !safeToAnmitsu || DM < Arf.minDt || DM > Arf.maxDt )
					continue;
				Arf.sHit += H.status & SPECIAL;

				if( DM < Arf.minDt )
					++Arf.early, H.status = EARLY_LIT;
				else if( DM <= Arf.maxDt )  [[likely]]
					++Arf.hHit, H.status = HIT_LIT;
				else
					++Arf.late, H.status = LATE_LIT;
				H.deltaMs = DM;
			}
	}
	else {
		for(const Info ei = Arf.eIdx[G];  Echo& E : std::span(Arf.echoes).subspan(ei.f, ei.c))
			if( const int32_t D = Arf.msTime - E.ms;  D < -370 )		break;
			else if									( D < +471 )		E = scanEcho(E, D, validTouches);
		for(const Info hi = Arf.hIdx[G];  Hint& H : std::span(Arf.hints).subspan(hi.f, hi.c))
			if( const int32_t D = Arf.msTime - H.ms;  D < -370 )		break;
			else if									( D < +471 )		H = scanHint(H, validTouches);
	}
}

void Ar::JudgeArfSweep() noexcept {
	for(const Info ei = Arf.eIdx[ Arf.msTime >> 10 ];  Echo& E : std::span(Arf.echoes).subspan(ei.f, ei.c))
		if( const int32_t DM = Arf.msTime - E.ms;  DM > 100 )						 /* [3] Lost Behavior */
			E.status >> 2 ?  0 : (Arf.lost += E.status & SPECIAL,  E.status = E.status << 1 & 2);
		else if( DM >= 0 )												/* [2] Echo Behavior · Catch Path */
			E.status >> 1 == 1 ? (Arf.eHit += E.status & SPECIAL,  E.status += HIT,  E.deltaMs = DM) : 0;
		else break;
	for(const Info hi = Arf.hIdx[ Arf.msTime >> 10 ];  Hint& H : std::span(Arf.hints).subspan(hi.f, hi.c))
		if( Arf.msTime - H.ms > 100 )
			H.status < HIT  ?  (Arf.lost++,  H.status = LOST) : 0;
		else break;
}

int Ar::JudgeArf(lua_State* L) noexcept {
	/* Usage:
	 * Arf4.JudgeArf(xs, ys, phases)
	 */
	Duo validTouches[33];
	uint8_t touchCount = 0, anyPressed = false, anyReleased = false;
	for( uint8_t i = 1 ; i < 33 ; i++ )
		switch( lua_rawgeti(L, 3, i), lua_tointeger(L, -1) ) {
			case 1:
				anyPressed = true;
			case 2:
				validTouches[touchCount].a = ( lua_rawgeti(L, 1, i), lua_tonumber(L,-1) );
				validTouches[touchCount].b = ( lua_rawgeti(L, 2, i), lua_tonumber(L,-1) );
				lua_pop(L, 3), touchCount++;
				break;
			case 3:
				anyReleased = true;
			[[likely]] default:
				lua_pop(L, 1);
		}
	return validTouches[touchCount].val = -1,   // Actually ~0
		   judgeArfInternal(validTouches, anyPressed, anyReleased), 0;
}
#endif