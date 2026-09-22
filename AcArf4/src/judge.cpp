//  Arf4 Judge  //
#ifndef AR_BUILD_VIEWER
#include <Arf4.h>

using namespace Ar;
static constexpr uint16_t OBJECT_SIZE = 448,
							HALF_SIZE = OBJECT_SIZE >> 1,  HAD = 2;
/* Judge Behaviors:
 * [1] Tap Behavior		[2] Echo Behavior		[3] Lost Behavior
 * 	   Earliest Path		Catch ~ Drag Path		Scored / Non-Scored
 */
static Duo validTs[33];
static uint8_t hasTouchNear(const int16_t cdx, const int8_t cdy) noexcept {
	const float l = (900.0f - HALF_SIZE) + cdx * Arf.xScale + Arf.xDelta,	r = l + OBJECT_SIZE,
				d = (540.0f - HALF_SIZE) + cdy * Arf.yScale,				u = d + OBJECT_SIZE;
	for( struct { uint8_t x:1 = Arf.isAnyX,  y:1 = Arf.isAnyY,  i:6; } H = {};  ~validTs[H.i].val;  ++H.i )
		if( const Duo T = validTs[H.i];  HAD == (H.x | T.a >= l & T.a <= r) + (H.y | T.b >= d & T.b <= u) )
			return HAD;
	return false;
}

static Body scanHt(Body hint) noexcept {
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

static Body scanEx(Body echo, const int16_t deltaMs) noexcept {
	if( echo.status & HIT )
		echo.status & 2  ?  echo.status += hasTouchNear(echo.cdx, echo.cdy) - HAD : 0;
	else if( echo.status & NJUDGED_LIT )								 /* [2] Echo Behavior · Drag Path */
		hasTouchNear(echo.cdx, echo.cdy) ? 0 :
			( deltaMs > -88  &&  deltaMs < 101 ) ?
				( echo.status += 2,  echo.deltaMs = deltaMs,  Arf.hit += (Arf.echoHit |= echo.status & 1) ):
				( echo.status &= SPECIAL );
	else
		echo.status += hasTouchNear(echo.cdx, echo.cdy);
	return echo;
}

static void judgeArfInternal(uint32_t minJmx) noexcept {
	if( const Index I = Arf.idx[ Arf.msTime >> 10 ];  minJmx & 1 /* Pressed */ ) {
		for( Body *B = Arf.echo + I.eSince;  B < Arf.ceil;  ++B )
			if( const int16_t D = Arf.msTime - B->ms;  D < -370 )		break;
			else if									 ( D > +370 )		{}			  /* [1] Tap Behavior */
			else if( Body &E = *B;  ( E = scanEx(E,D) ).status == SPECIAL_LIT  &&  D > -101  &  D < 101 ) {
				if( minJmx < 0x80 )   // xCnt [2,127]
					minJmx = E.ms;
				else if( minJmx != E.ms )
					continue;
				Arf.hit += ( Arf.echoHit |= SPECIAL ),
				E.deltaMs = D,  E.status |= HIT;
			}
		for( Body *B = Arf.hint + I.hSince;  B < Arf.echo;  ++B )
			if ( int16_t D = Arf.msTime - B->ms;  D < -370 )			break;
			else if								( D > +370 )			{}
			else if( Body &H = *B;  ( H = scanHt(H) ).status >> 1 == 1  &&  D > -101  &  D < 101 ) {
				if( minJmx > 0x7F  &&  minJmx < H.ms )
					continue;
				Arf.sHit += H.status & ( Arf.hintHit |= SPECIAL );

				if( H.deltaMs = D,  (D -= InputDelta) + Arf.judgeZone < 0 )
					++Arf.early, H.status = HLIT_EC;
				else if( D - Arf.judgeZone <= 0 )  [[likely]]
					++Arf.hit,	 H.status = HLIT;
				else
					++Arf.late,  H.status = HLIT_EC;
				minJmx = H.ms;
			}
	} else {
		for( Body *E = Arf.echo + I.eSince;  E < Arf.ceil;  ++E )
			if( const int16_t D = Arf.msTime - E->ms;  D < -370 )		break;
			else if									 ( D < +371 )		*E = scanEx(*E, D);
		for( Body *H = Arf.hint + I.hSince;  H > Arf.echo;  ++H )
			if( const int16_t D = Arf.msTime - H->ms;  D < -370 )		break;
			else if									 ( D < +371 )		*H = scanHt(*H);
	}
}

void Ar::JudgeArfSweep() noexcept {
	const Index I = Arf.idx[ Arf.msTime >> 10 ];
	for( Body *B = Arf.echo + I.eSince;  B < Arf.ceil;  ++B )
		if( const int16_t D = Arf.msTime - B->ms;  D < 0 )				break;
		else if( Body &E = *B;  E.status >> 1 == 1 )					/* [2] Echo Behavior · Catch Path */
			Arf.hit += ( Arf.echoHit |= E.status & 1 ),  E.status |= HIT;
		else if( 1 == (E.status | E.deltaMs)  &&  D > 100 )							 /* [3] Lost Behavior */
			Arf.lost++,  E.deltaMs = 2;
	for( Body *B = Arf.hint + I.hSince;  B < Arf.echo;  ++B )
		if( Body &H = *B;  Arf.msTime - H.ms > 100 )
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
				xCnt |= 1;
			case 2:   // OnScreen
				validTs[xCnt >> 1] = { .a = (float)( lua_rawgeti(L,1,i), lua_tonumber(L,5) ) ,
									   .b = (float)( lua_rawgeti(L,2,i), lua_tonumber(L,6) ) },  xCnt += 2;
			[[likely]] default:
				CLEAR: lua_settop(L,3);
		}
	return validTs[xCnt >> 1].val = ~(0ll),
		   judgeArfInternal(xCnt),
		   0;
}
#endif