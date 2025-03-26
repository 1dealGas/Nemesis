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
			const float l = (900.0f - HALF_SIZE) + cdx * Arf.xScale + Arf.xDelta, r = l + OBJECT_SIZE;
			const float d = (540.0f - HALF_SIZE) + cdy * Arf.yScale, u = d + OBJECT_SIZE;
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
		case NJUDGED:
		case NJUDGED_LIT:
			hint.status = hasTouchNear(hint.cdx, hint.cdy, validTouches) ? NJUDGED_LIT : NJUDGED ;
			return hint;
		case SPECIAL:
		case SPECIAL_LIT:
			hint.status = hasTouchNear(hint.cdx, hint.cdy, validTouches) ? SPECIAL_LIT : SPECIAL ;
			return hint;
		case EARLY_LIT:
			if(! hasTouchNear(hint.cdx, hint.cdy, validTouches) )
				hint.status = EARLY;
			return hint;
		case HIT_LIT:
			if(! hasTouchNear(hint.cdx, hint.cdy, validTouches) )
				hint.status = HIT;
			return hint;
		case LATE_LIT:
			if(! hasTouchNear(hint.cdx, hint.cdy, validTouches) )
				hint.status = LATE;
		default:
			return hint;
	}
}

static Echo scanEcho(Echo echo, const int32_t deltaMs, const Duo validTouches[]) noexcept {
	switch( echo.status ) {
		case NJUDGED:
			if( hasTouchNear(echo.cdx, echo.cdy, validTouches) )
				echo.status = NJUDGED_LIT;
			return echo;
		case SPECIAL:
			if( hasTouchNear(echo.cdx, echo.cdy, validTouches) )
				echo.status = SPECIAL_LIT;
			return echo;
		case HIT_LIT:
			echo.status = hasTouchNear(echo.cdx, echo.cdy, validTouches)  ?  HIT_LIT : HIT ;
			return echo;

		/* [2] Echo Behavior -- Drag Path */
		case NJUDGED_LIT:
			if(! hasTouchNear(echo.cdx, echo.cdy, validTouches) )
				if( deltaMs >= -100  &&  deltaMs <= 100 /* In case of Sweep Delay */ )
					echo.status = HIT,			 echo.deltaMs = deltaMs;
				else
					echo.status = NJUDGED;
			return echo;
		case SPECIAL_LIT:
			if(! hasTouchNear(echo.cdx, echo.cdy, validTouches) )
				if( deltaMs >= -73 /* In case of Piercing */  &&  deltaMs <= 100 )
					echo.status = HIT,			 echo.deltaMs = deltaMs,			 Arf.eHit++;
				else
					echo.status = SPECIAL;
		default:
			return echo;
	}
}

#include <span>
#include <dmsdk/dlib/time.h>
static void judgeArfInternal(const Duo validTouches[], const bool anyPressed, const bool anyRel) noexcept {
	const uint64_t msTime = Arf.msTime + dmTime::GetMonotonicTime() - UsysTime,
						G = msTime >> 10;
	if( anyRel )
		blockedPos.clear();
	if( uint32_t minJudgedMs = NULL;  anyPressed ) {
		for(const Info ei = Arf.eIdx[G];  Echo& E : std::span(Arf.echoes).subspan(ei.f, ei.c)) {
			const int32_t deltaMs = Arf.msTime - E.ms;
			if( deltaMs < -370 )		break;
			if( deltaMs > +470 )		continue;
			E = scanEcho(E, deltaMs, validTouches);

			if( E.status == NJUDGED_LIT  ||  E.status == SPECIAL_LIT )
				if( deltaMs > -101  &&  deltaMs < 101 ) {	/* [1] Tap Behavior · Earliest & Anmitsu Path */
					const bool safeToAnmitsu = testAnmitsuSafety(E.cdx, E.cdy, E.status==SPECIAL_LIT);
					if( !minJudgedMs )
						minJudgedMs = E.ms;
					else if( minJudgedMs != E.ms )				  // Consider if maxDt < 0
						if( !safeToAnmitsu || deltaMs < Arf.minDt || deltaMs > Arf.maxDt )
							continue;
					Arf.eHit += (E.status == SPECIAL_LIT), E.status = HIT_LIT;
					E.deltaMs = deltaMs;
				}
		}
		for(const Info hi = Arf.hIdx[G];  Hint& H : std::span(Arf.hints).subspan(hi.f, hi.c)) {
			const int32_t deltaMs = Arf.msTime - H.ms;
			if( deltaMs < -370 )		break;
			if( deltaMs > +470 )		continue;
			H = scanHint(H, validTouches);

			if( H.status == NJUDGED_LIT  ||  H.status == SPECIAL_LIT )
				if( deltaMs > -101  &&  deltaMs < 101 ) {
					const bool safeToAnmitsu = testAnmitsuSafety(H.cdx, H.cdy, true);
					if( !minJudgedMs  ||  minJudgedMs >= H.ms )
						minJudgedMs = H.ms;
					else if( !safeToAnmitsu || deltaMs < Arf.minDt || deltaMs > Arf.maxDt )
						continue;
					Arf.sHit += (H.status == SPECIAL_LIT);

					if( deltaMs < Arf.minDt )
						++Arf.early, H.status = EARLY_LIT;
					else if( deltaMs <= Arf.maxDt )  [[likely]]
						++Arf.hHit, H.status = HIT_LIT;
					else
						++Arf.late, H.status = LATE_LIT;
					H.deltaMs = deltaMs;
				}
		}
	}
	else {
		for(const Info ei = Arf.eIdx[G];  Echo& E : std::span(Arf.echoes).subspan(ei.f, ei.c)) {
			const int32_t deltaMs = Arf.msTime - E.ms;
			if( deltaMs < -370 )		break;
			if( deltaMs > +470 )		continue;
			E = scanEcho(E, deltaMs, validTouches);
		}
		for(const Info hi = Arf.hIdx[G];  Hint& H : std::span(Arf.hints).subspan(hi.f, hi.c)) {
			const int32_t deltaMs = Arf.msTime - H.ms;
			if( deltaMs < -370 )		break;
			if( deltaMs > +470 )		continue;
			H = scanHint(H, validTouches);
		}
	}
}

void Ar::JudgeArfSweep() noexcept {
	const uint16_t G = Arf.msTime >> 10;
	for(const Info ei = Arf.eIdx[G];  Echo& E : std::span(Arf.echoes).subspan(ei.f, ei.c))
		if( const int32_t deltaMs = Arf.msTime - E.ms;  deltaMs > 255 ) [[unlikely]] {}
		else if( deltaMs > 100 )													 /* [3] Lost Behavior */
			switch( E.status ) {
				case SPECIAL: case SPECIAL_LIT:		E.status = SPECIAL_LOST;  Arf.lost++;  break;
				case NJUDGED: case NJUDGED_LIT:		E.status = LOST;
				default:;
			}
		else if( deltaMs >= 0 )											/* [2] Echo Behavior · Catch Path */
			switch( E.status ) {
				case SPECIAL_LIT:					Arf.eHit++;
				case NJUDGED_LIT:					E.status = HIT_LIT, E.deltaMs = deltaMs;
				default:;
			}
		else break;
	for(const Info hi = Arf.hIdx[G];  Hint& H : std::span(Arf.hints).subspan(hi.f, hi.c))
		if( const int32_t deltaMs = Arf.msTime - H.ms;  deltaMs > 255 ) [[unlikely]] {}
		else if( deltaMs > 100 )
			H.status < HIT  ?  H.status = LOST, Arf.lost++ : 0;
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