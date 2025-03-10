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
	switch( Arf.isAnyX | Arf.isAnyY<<1 ) {
		case 3:
			return true;
		case 2: /* isAnyY */ {
			uint8_t whichTouch = 0;
			const float d = (540.0f - HALF_SIZE) + cdy * Arf.yScale, u = d + OBJECT_SIZE;
			while(~ validTouches[whichTouch].val ) {   // Using {.a=NaN, .b=NaN} as the ending identifier
				const float touchY = validTouches[whichTouch].b;
				if( touchY >= d  &&  touchY <= u )
					return true;
				++whichTouch;
			}
			return false;
		}
		case 1: /* isAnyX */ {
			uint8_t whichTouch = 0;
			const float l = (900.0f - HALF_SIZE) + cdx * Arf.xScale + Arf.xDelta, r = l + OBJECT_SIZE;
			while(~ validTouches[whichTouch].val ) {
				const float touchX = validTouches[whichTouch].a;
				if( touchX >= l  &&  touchX <= r )
					return true;
				++whichTouch;
			}
			return false;
		}
		[[likely]] default: {
			uint8_t whichTouch = 0;
			const float l = (900.0f - HALF_SIZE) + cdx * Arf.xScale + Arf.xDelta, r = l + OBJECT_SIZE;
			const float d = (540.0f - HALF_SIZE) + cdy * Arf.yScale, u = d + OBJECT_SIZE;
			while(~ validTouches[whichTouch].val ) {
				const float touchX = validTouches[whichTouch].a;
				if( touchX >= l  &&  touchX <= r ) {
					const float touchY = validTouches[whichTouch].b;
					if( touchY >= d  &&  touchY <= u )
						return true;
				}
				++whichTouch;
			}
			return false;
		}
	}
}

#include <vector>
static std::vector<Duo> blockPos;
static bool testAnmitsuSafety(const int16_t cdx, const int16_t cdy) noexcept {
	const float x = 900.0f + cdx * Arf.xScale + Arf.xDelta,  y = 540.0f + cdy * Arf.yScale;
	const float l = x - OBJECT_SIZE,  r = x + OBJECT_SIZE,  d = y - OBJECT_SIZE,  u = y + OBJECT_SIZE;

	switch( Arf.isAnyX | Arf.isAnyY<<1 ) {
		case 3:
			return false;
		case 2: /* isAnyY */
			for(const auto i : blockPos)
				if( i.a > l && i.a < r )
					return false;
			break;
		case 1: /* isAnyX */
			for(const auto i : blockPos)
				if( i.b > d && i.b < u )
					return false;
			break;
		[[likely]] default:
			for(const auto i : blockPos)
				if( i.a > l && i.a < r )
					if( i.b > d && i.b < u )
						return false;
	}

	blockPos.push_back({ .a = x, .b = y });   // Push "Safe when Anmitsu" Hints
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
			echo.status = hasTouchNear(echo.cdx, echo.cdy, validTouches) ? HIT_LIT : HIT ;
			return echo;

		/* [2] Echo Behavior -- Drag Path */
		case NJUDGED_LIT:
			if(! hasTouchNear(echo.cdx, echo.cdy, validTouches) )
				if( deltaMs >= -100  &&  deltaMs <= 100 )
					echo.status = HIT,			 echo.deltaMs = deltaMs;
				else
					echo.status = NJUDGED;
			return echo;
		case SPECIAL_LIT:
			if(! hasTouchNear(echo.cdx, echo.cdy, validTouches) )
				if( deltaMs >= -100  &&  deltaMs <= 100 )
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
	using Span = std::span;
	const uint64_t msTime = Arf.msTime + dmTime::GetMonotonicTime() - UsysTime, G = msTime >> 9;

	if(anyRel)
		blockPos.clear();
	if(uint32_t minJudgedMs = NULL;  anyPressed) {
		for(const Info eIdx = Arf.eIdx[G];  Echo& echo : Span( Arf.echoes.begin() + eIdx.f, eIdx.c )) {
			const int32_t deltaMs = Arf.msTime - echo.ms;
			if( deltaMs < -370 )		break;
			if( deltaMs > +470 )		continue;
			echo = scanEcho(echo, deltaMs, validTouches);

			if( echo.status == NJUDGED_LIT  ||  echo.status == SPECIAL_LIT )
				if( deltaMs > -101  &&  deltaMs < 101 ) {
					/* [1] Tap Behavior -- Earliest & Anmitsu Path */
					const bool safeToAnmitsu = testAnmitsuSafety(echo.cdx, echo.cdy);
					if( !minJudgedMs )
						minJudgedMs = echo.ms;
					else if( minJudgedMs != echo.ms )			  // Consider if maxDt < 0
						if( !safeToAnmitsu || deltaMs < Arf.minDt || deltaMs > Arf.maxDt )
							continue;
					echo.status = HIT_LIT, echo.deltaMs = deltaMs;
					Arf.eHit += (echo.status == SPECIAL_LIT);
				}
		}
		for(const Info hIdx = Arf.hIdx[G];  Hint& hint : Span( Arf.hints.begin() + hIdx.f, hIdx.c )) {
			const int32_t deltaMs = Arf.msTime - hint.ms;
			if( deltaMs < -370 )		break;
			if( deltaMs > +470 )		 continue;
			hint = scanHint(hint, validTouches);

			if( hint.status == NJUDGED_LIT  ||  hint.status == SPECIAL_LIT )
				if( deltaMs > -101  &&  deltaMs < 101 ) {
					const bool safeToAnmitsu = testAnmitsuSafety(hint.cdx, hint.cdy);
					if( !minJudgedMs  ||  minJudgedMs >= hint.ms )
						minJudgedMs = hint.ms;
					else if( !safeToAnmitsu || deltaMs < Arf.minDt || deltaMs > Arf.maxDt )
						continue;
					Arf.sHit += (hint.status == SPECIAL_LIT);

					if( deltaMs < Arf.minDt )
						++Arf.early, hint.status = EARLY_LIT;
					else if( deltaMs <= Arf.maxDt )
						++Arf.hHit, hint.status = HIT_LIT;
					else
						++Arf.late, hint.status = LATE_LIT;
					hint.deltaMs = deltaMs;
				}
		}
	}
	else {
		for(const Info eIdx = Arf.eIdx[G];  Echo& echo : Span( Arf.echoes.begin() + eIdx.f, eIdx.c )) {
			const int32_t deltaMs = Arf.msTime - echo.ms;
			if( deltaMs < -370 )		break;
			if( deltaMs > +470 )		continue;
			echo = scanEcho(echo, deltaMs, validTouches);
		}
		for(const Info hIdx = Arf.hIdx[G];  Hint& hint : Span( Arf.hints.begin() + hIdx.f, hIdx.c )) {
			const int32_t deltaMs = Arf.msTime - hint.ms;
			if( deltaMs < -370 )		break;
			if( deltaMs > +470 )		continue;
			hint = scanHint(hint, validTouches);
		}
	}
}

void Ar::JudgeArfSweep() noexcept {
	const uint32_t G = Arf.msTime >> 9;
	for( const Info eIdx = Arf.eIdx[G];  Echo& echo : std::span( Arf.echoes.begin() + eIdx.f, eIdx.c ))
		if( const int32_t deltaMs = Arf.msTime - echo.ms;  deltaMs > 255 ) {}
		else if( deltaMs > 100 )												   /* [3] Lost Behavior */
			switch( echo.status ) {
				case SPECIAL: case SPECIAL_LIT:		echo.status = SPECIAL_LOST;  Arf.lost++;  break;
				case NJUDGED: case NJUDGED_LIT:		echo.status = LOST;
				default:;
			}
		else if( deltaMs >= 0 )										 /* [2] Echo Behavior -- Catch Path */
			switch( echo.status ) {
				case SPECIAL_LIT:					Arf.eHit++;
				case NJUDGED_LIT:					echo.status = HIT_LIT, echo.deltaMs = deltaMs;
				default:;
			}
		else break;

	for( const Info hIdx = Arf.hIdx[G];  Hint& hint : std::span( Arf.hints.begin() + hIdx.f, hIdx.c ))
		if( const int32_t deltaMs = Arf.msTime - hint.ms;  deltaMs > 255 ) {}
		else if( deltaMs > 100 )
			switch( hint.status ) {
				case SPECIAL: case SPECIAL_LIT:		hint.status = SPECIAL_LOST;	Arf.lost++;  break;
				case NJUDGED: case NJUDGED_LIT:		hint.status = LOST;			Arf.lost++;
				default:;
			}
		else break;
}

int Ar::JudgeArf(lua_State* L) noexcept {
	/* Usage:
	 * Arf4.JudgeArf(xs, ys, phases)
	 */
	Duo validTouches[33];
	uint8_t touchCount = 0, anyPressed = false, anyReleased = false;
	for( uint8_t i = 1 ; i < 33 ; i++ ) {
		switch( lua_rawgeti(L, i, 3), lua_tointeger(L, -1) ) {
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
	}
	return validTouches[touchCount].val = -1,   // Actually ~0
		   judgeArfInternal(validTouches, anyPressed, anyReleased), 0;
}
#endif