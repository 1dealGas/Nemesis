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
							HALF_SIZE = OBJECT_SIZE >> 1,  HAD = 2;
static uint8_t hasTouchNear(const int16_t cdx, const int16_t cdy, const Duo validTs[]) noexcept {
	switch( uint8_t whichTouch = 0;  Arf.isAnyX | Arf.isAnyY<<1 ) {
		case 3:
			return HAD;
		case 2: /* isAnyY */
			for( const float l = (900.0f - HALF_SIZE) + cdx * Arf.xScale + Arf.xDelta,  r = l + OBJECT_SIZE;
				 ~validTs[whichTouch].val;  ++whichTouch )   // {.a=NaN, .b=NaN} as the ending identifier
				if( const float touchX = validTs[whichTouch].a;  touchX >= l  &&  touchX <= r )
					return HAD;
			return false;
		case 1: /* isAnyX */
			for( const float d = (540.0f - HALF_SIZE) + cdy * Arf.yScale,  u = d + OBJECT_SIZE;
				~validTs[whichTouch].val;  ++whichTouch )
				if( const float touchY = validTs[whichTouch].b;  touchY >= d  &&  touchY <= u )
					return HAD;
			return false;
		[[likely]] default:
			for( const float l = (900.0f - HALF_SIZE) + cdx * Arf.xScale + Arf.xDelta,	r = l + OBJECT_SIZE,
							 d = (540.0f - HALF_SIZE) + cdy * Arf.yScale,				u = d + OBJECT_SIZE;
				 ~validTs[whichTouch].val;  ++whichTouch )
				if( const Duo T = validTs[whichTouch];  T.a >= l  &&  T.a <= r  &&  T.b >= d  &&  T.b <= u )
					return HAD;
			return false;
	}
}

static std::vector<Duo> blockedPos;
static bool testAnmitsuSafety(const int16_t cdx, const int16_t cdy, const bool isScored) noexcept {
	const float x = 900.0f + cdx * Arf.xScale + Arf.xDelta,  y = 540.0f + cdy * Arf.yScale,
				l = x - OBJECT_SIZE,  r = x + OBJECT_SIZE,   d = y - OBJECT_SIZE,  u = y + OBJECT_SIZE;
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

static Body scanHint(Body hint, const Duo validTouches[]) noexcept {
	switch( hint.status ) {
		case SPECIAL:
			if( hint.deltaMs )
		/**/return hint;
		case NJUDGED:		case NJUDGED_LIT:		case SPECIAL_LIT:
			hint.status  =  hasTouchNear(hint.cdx, hint.cdy, validTouches) + (hint.status & SPECIAL);
			return hint;
		case HLIT:			case HLIT_EC:
			hint.status +=  hasTouchNear(hint.cdx, hint.cdy, validTouches) - HAD;
		default:
			return hint;
	}
}

static Body scanEcho(Body echo, const int32_t deltaMs, const Duo validTouches[]) noexcept {
	if( echo.status & HIT )
		echo.status & 2  ?  echo.status += hasTouchNear(echo.cdx, echo.cdy, validTouches) - HAD : 0;
	else if( echo.status & NJUDGED_LIT )								 /* [2] Echo Behavior · Drag Path */
		hasTouchNear(echo.cdx, echo.cdy, validTouches) ? 0 :
			( deltaMs > -88  &&  deltaMs < 101 ) ?
				( echo.status += 2,  echo.deltaMs = deltaMs,  Arf.eHit += echo.status & SPECIAL ):
				( echo.status &= SPECIAL );
	else
		echo.status += hasTouchNear(echo.cdx, echo.cdy, validTouches);
	return echo;
}

#include <span>
static void judgeArfInternal(const Duo validTouches[], const bool anyPressed, const bool anyRel) noexcept {
	if( anyRel )
		blockedPos.clear();
	if( const Index I = Arf.idx[ Arf.msTime >> 10 ];  anyPressed ) {
		uint32_t minJudgedMs = NULL;
		for(Body& E : std::span(Arf.echoes).subspan(I.eSince))
			if( const int32_t DM = Arf.msTime - E.ms;  DM < -370 )		break;
			else if									 ( DM > +470 )		{}			  /* [1] Tap Behavior */
			else if( E = scanEcho(E, DM, validTouches),  E.status >> 1 == 1  &&  DM > -101  &&  DM < 101 ) {
				const bool safeToAnmitsu = testAnmitsuSafety(E.cdx, E.cdy, E.status & SPECIAL);
				if( !minJudgedMs )
					minJudgedMs = E.ms;
				else if( minJudgedMs != E.ms  &&  !safeToAnmitsu )
					continue;
				Arf.eHit += E.status & SPECIAL,  E.status |= HIT;
				E.deltaMs = DM;
			}
		for(Body& H : std::span(Arf.hints).subspan(I.hSince))
			if( const int32_t DM = Arf.msTime - H.ms;  DM < -370 )		break;
			else if									 ( DM > +470 )		{}
			else if( H = scanHint(H, validTouches),  H.status >> 1 == 1  &&  DM > -101  &&  DM < 101 ) {
				const bool safeToAnmitsu = testAnmitsuSafety(H.cdx, H.cdy, true);
				if( !minJudgedMs  ||  minJudgedMs >= H.ms )
					minJudgedMs = H.ms;
				else if( !safeToAnmitsu )
					continue;
				Arf.sHit += H.status & SPECIAL;

				if( DM < Arf.minDt )
					++Arf.early, H.status = HLIT_EC;
				else if( DM <= Arf.maxDt )  [[likely]]
					++Arf.hHit,  H.status = HLIT;
				else
					++Arf.late,  H.status = HLIT_EC;
				H.deltaMs = DM;
			}
	}
	else {
		for(Body& E : std::span(Arf.echoes).subspan(I.eSince))
			if( const int32_t D = Arf.msTime - E.ms;  D < -370 )		break;
			else if									( D < +471 )		E = scanEcho(E, D, validTouches);
		for(Body& H : std::span(Arf.hints).subspan(I.hSince))
			if( const int32_t D = Arf.msTime - H.ms;  D < -370 )		break;
			else if									( D < +471 )		H = scanHint(H, validTouches);
	}
}

void Ar::JudgeArfSweep() noexcept {
	const Index I = Arf.idx[ Arf.msTime >> 10 ];
	for( Body& E : std::span(Arf.echoes).subspan(I.eSince) )
		if( const int32_t DM = Arf.msTime - E.ms;  DM < 0 )				break;
		else if( E.status >> 1 == 1 )									/* [2] Echo Behavior · Catch Path */
			Arf.eHit += E.status & SPECIAL,  E.status |= HIT;
		else if((E.status | E.deltaMs) == 1  &&  DM > 100 )							 /* [3] Lost Behavior */
			Arf.lost++, E.deltaMs = 2;
	for( Body& H : std::span(Arf.hints).subspan(I.hSince) )
		if( Arf.msTime - H.ms > 100 )
			(H.status & HIT) || (H.deltaMs) ? 0 : ( ++Arf.lost,  H.status = H.deltaMs = 1 );
		else break;
}

int Ar::JudgeArf(lua_State* L) noexcept {
	/* Usage:
	 * Arf4.JudgeArf(xs, ys, phases)
	 */
	Duo validTouches[33];
	uint8_t touchCount = 0, anyPressed = false, anyReleased = false;
	for( uint8_t i = 1;  i < 33;  i++ )
		switch( lua_rawgeti(L,3,i), lua_tointeger(L,4) ) {
			case 1:
				anyPressed = true;
			case 2:
				validTouches[touchCount].a = ( lua_rawgeti(L,1,i), lua_tonumber(L,5) );
				validTouches[touchCount].b = ( lua_rawgeti(L,2,i), lua_tonumber(L,6) ),  touchCount++;
				goto CLEAR;
			case 3:
				anyReleased = true;
			[[likely]] default:
				CLEAR: lua_settop(L,3);
		}
	return validTouches[touchCount].val = ~(0ll),
		   judgeArfInternal(validTouches, anyPressed, anyReleased), 0;
}
#endif