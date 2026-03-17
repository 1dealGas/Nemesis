//  Arf4 Judge  //
#ifndef AR_BUILD_VIEWER
#include <Arf4.h>

using namespace Ar;
static constexpr uint16_t OBJECT_SIZE = 448,
							HALF_SIZE = OBJECT_SIZE >> 1,  HAD = 2;
/* Judge Behaviors:
 * [1] Tap Behavior
 * 	   Earliest Path / Anmitsu Path
 * [2] Echo Behavior
 *     Catch Path / Drag Path
 * [3] Lost Behavior
 */
static Duo validTs[33];
static uint8_t hasTouchNear(const int16_t cdx, const int16_t cdy) noexcept {
	const float l = (900.0f - HALF_SIZE) + cdx * Arf.xScale + Arf.xDelta,	r = l + OBJECT_SIZE,
				d = (540.0f - HALF_SIZE) + cdy * Arf.yScale,				u = d + OBJECT_SIZE;
	for( struct { uint8_t x:1 = Arf.isAnyX,  y:1 = Arf.isAnyY,  i:6; }  H = {};  ~validTs[H.i].val;  ++H.i )
		if( const Duo T = validTs[H.i];  HAD == (H.x | T.a >= l & T.a <= r) + (H.y | T.b >= d & T.b <= u) )
			return HAD;
	return false;
}

static std::vector<Duo> blockedPos;
static bool testAnmitsuSafety(const int16_t cdx, const int16_t cdy, const bool isScored) noexcept {
	const float x = 900.0f + cdx * Arf.xScale + Arf.xDelta,  y = 540.0f + cdy * Arf.yScale,
				l = x - OBJECT_SIZE,  r = x + OBJECT_SIZE,   d = y - OBJECT_SIZE,  u = y + OBJECT_SIZE;
	for( struct { uint8_t x:1 = Arf.isAnyX,  y:1 = Arf.isAnyY; } const H = {};  const auto P : blockedPos )
		if( HAD == (H.x | P.a >= l & P.a <= r) + (H.y | P.b >= d & P.b <= u) )
			return false;
	if( isScored )   // Push "Safe when Anmitsu" Objects
		blockedPos.push_back({ .a = x, .b = y });
	return true;
}

static Body scanHint(Body hint) noexcept {
	switch( hint.status ) {
		case SPECIAL:
			if( hint.deltaMs == 0 )
		case NJUDGED:		case NJUDGED_LIT:		case SPECIAL_LIT:
		/**/hint.status  =  hasTouchNear(hint.cdx, hint.cdy) + (hint.status & SPECIAL);
			return hint;
		case HLIT:			case HLIT_EC:
			hint.status +=  hasTouchNear(hint.cdx, hint.cdy) - HAD;
		default:
			return hint;
	}
}

static Body scanEcho(Body echo, const int32_t deltaMs) noexcept {
	if( echo.status & HIT )
		echo.status & 2  ?  echo.status += hasTouchNear(echo.cdx, echo.cdy) - HAD : 0;
	else if( echo.status & NJUDGED_LIT )								 /* [2] Echo Behavior · Drag Path */
		hasTouchNear(echo.cdx, echo.cdy) ? 0 :
			( deltaMs > -88  &&  deltaMs < 101 ) ?
				( echo.status += 2,  echo.deltaMs = deltaMs,  Arf.eHit += echo.status & SPECIAL ):
				( echo.status &= SPECIAL );
	else
		echo.status += hasTouchNear(echo.cdx, echo.cdy);
	return echo;
}

#include <span>
static void judgeArfInternal(const uint8_t anyRelPrs) noexcept {
	if( anyRelPrs & 1 )
		blockedPos.clear();
	if( const Index I = Arf.idx[ Arf.msTime >> 10 ];  anyRelPrs & 2 ) {
		uint32_t minJudgedMs = NULL;
		for(Body& E : std::span(Arf.echoes).subspan(I.eSince))
			if( const int32_t DM = Arf.msTime - E.ms;  DM < -370 )		break;
			else if									 ( DM > +470 )		{}			  /* [1] Tap Behavior */
			else if( E = scanEcho(E, DM),  E.status >> 1 == 1  &&  DM > -101  &&  DM < 101 ) {
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
			else if( H = scanHint(H),  H.status >> 1 == 1  &&  DM > -101  &&  DM < 101 ) {
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
			else if									( D < +471 )		E = scanEcho(E, D);
		for(Body& H : std::span(Arf.hints).subspan(I.hSince))
			if( const int32_t D = Arf.msTime - H.ms;  D < -370 )		break;
			else if									( D < +471 )		H = scanHint(H);
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
	uint8_t xCnt = 0;
	for( uint8_t i = 1;  i < 33;  ++i )
		switch( lua_rawgeti(L,3,i), lua_tointeger(L,4) ) {
			case 1:   // Pressed
				xCnt |= 2;
			case 2:   // OnScreen
				validTs[xCnt >> 2] = { .a = (float)( lua_rawgeti(L,1,i), lua_tonumber(L,5) ) ,
									   .b = (float)( lua_rawgeti(L,2,i), lua_tonumber(L,6) ) },  xCnt += 4;
				goto CLEAR;
			case 3:   // Released
				xCnt |= 1;
			[[likely]] default:
				CLEAR: lua_settop(L,3);
		}
	return validTs[xCnt >> 2].val = ~(0ll),
		   judgeArfInternal(xCnt),
		   0;
}
#endif