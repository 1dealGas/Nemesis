//  Arf4 Update  //
#include <Arf4.h>
#include <dmsdk/dlib/time.h>
#include <unordered_map>
#include <algorithm>
#include <span>

typedef dmGameObject::HInstance GO;				using namespace Ar;
typedef dmVMath::Vector3 v3i, *v3;				typedef dmVMath::Point3 P3;
typedef dmVMath::Vector4 v4i, *v4;				typedef dmVMath::Quat Qt;

struct AuInfo {
	uint64_t frameDt:10 = 0, aUsed:11 = 0, wUsed:10 = 0, eUsed:10 = 0, xUsed:10 = 0, hUsed:9 = 0;
	uint64_t sType:2 = false, playH:1 = false, playE:1 = false;
};


/* Utils & Render Methods */
static const Qt maxQuat(0.0, 0.0, 0.594822786751341, 0.803856860617217);
static Qt rotationToQuat(const float degree) noexcept {
	const auto cosSin = CosSin({ .a = degree * 0.5f });
	return Qt(0, 0, cosSin.b, cosSin.a);
}

static std::unordered_map<uint64_t, int16_t> lastWgo;
static AuInfo renderWish(lua_State* L, AuInfo info, Duo Pos, Duo zw) {
	if( Pos.b = 540 + Pos.b * Arf.yScale,  Pos.b >= -36  &&  Pos.b <= 1116 )
		if( Pos.a = 900 + Pos.a * Arf.xScale + Arf.xDelta,  Pos.a >= -36  &&  Pos.a <= 1836 )
			if( const uint16_t idx = lastWgo[Pos.val];  idx == 0 ) {
				lastWgo[Pos.val] = ++info.wUsed;
				const auto wGo = ( lua_rawgeti(L, WGO, info.wUsed),
										   dmScript::CheckGOInstance(L, -1) );
				SetPosition( wGo, P3(Pos.a, Pos.b, zw.a) );
				SetScale   ( wGo, (zw.b = 1 - zw.b,  0.637f + 0.437f * zw.b * zw.b) );

				// Tint
				lua_pushnumber(L, info.sType ? -zw.b : zw.b), lua_rawseti(L, WTINT, info.wUsed);
				lua_pop(L, 1);
			}
			else {
				const auto wGo = ( lua_rawgeti(L, WGO, idx),
										   dmScript::CheckGOInstance(L, -1) );
				SetPosition( wGo, GetPosition(wGo).setZ(0.01f) );
				SetScale   ( wGo, 0.637f );

				// Tint
				if(( lua_rawgeti(L, WTINT, idx), lua_tonumber(L, -1) ) < 0)
					lua_pushnumber(L, -1), lua_rawseti(L, WTINT, idx);
				else
					lua_pushnumber(L, info.sType ? -1 : 1), lua_rawseti(L, WTINT, idx);
				lua_pop(L, 2);
			}
	return info;
}

static AuInfo renderEcho(AuInfo info) {
	return info;
}

static AuInfo renderEchoHelper(AuInfo info) {
	return info;
}

static AuInfo renderAnim(lua_State* L, AuInfo info, Duo Pos, const int16_t msPast) {
	if( msPast > 370 )			return info;
	const auto tint = ( lua_rawgeti(L, ATINT, ++info.aUsed), dmScript::CheckVector4(L, -1) );
	const auto lAgo = ( lua_rawgeti(L, AL, info.aUsed), dmScript::CheckGOInstance(L, -1) ),
			   rAgo = ( lua_rawgeti(L, AR, info.aUsed), dmScript::CheckGOInstance(L, -1) );
	lua_pop(L, 3);

	// Tint
	tint -> setXYZ( Arf.aTint[info.sType] );
	if( double w;  msPast < 73 )
		w = msPast * 0.01,			tint -> setW( 0.17199 + 0.637 * w * (2.0-w) );
	else
		w = (msPast-73) / 297.0,	tint -> setW( 0.637 * (1.0f - w*w) );

	// Transform
	const float PosZ = msPast * 0.0001;
	SetPosition(lAgo, P3( Pos.a, Pos.b, PosZ ));
	SetPosition(rAgo, P3( Pos.a, Pos.b, PosZ + 0.00005 ));

	if( double leftRatio;  msPast < 193 )
		leftRatio = msPast / 193.0,
		SetRotation( lAgo, rotationToQuat(45.0 + 28.0 * leftRatio) ),
		SetScale( lAgo, 1.0f + 0.637f * leftRatio * (2.0 - leftRatio) );
	else
		SetRotation(lAgo, maxQuat),
		SetScale(lAgo, 1.637f);

	const double rightRatio = msPast / 370.0;
	SetRotation( rAgo, rotationToQuat(45.0 - 8.0 * rightRatio) );
	SetScale( rAgo, 1.0f + 0.637f * rightRatio * (2.0 - rightRatio) );
	return info;
}

static constexpr auto dtPred = [](const Delta a, const Delta b) noexcept {
	return a.t < b.t;
};


/* Main */
using Span = std::span;
int Ar::UpdateArf(lua_State* L) noexcept {
	/* Usage:
	 * local wgo_used, hgo_used, ego_used, ehgo_used, ago_used, h_playhs, e_playhs = Arf4.UpdateArf(
	 *       ms, dt, wgos, hgos, egos, ehgos, agols, agors, wtints, htints, etints, ehtints, atints)
	 */
	Arf.msTime = lua_tointeger(L, 1);
		if( Arf.msTime >= Arf.before )			return 0;
			Arf.msTime = max(Arf.msTime, 2);	UsysTime = dmTime::GetMonotonicTime();
	#ifndef AR_BUILD_VIEWER
		if (! Arf.isAuto )						JudgeArfSweep();
	#endif

	/* Info */
	const double eSpeed = (PlayerSpeed * Arf.cSpeed + 11) / 1500.0, echoDt = Arf.msTime * eSpeed,
				 dSpeed = eSpeed / 1024 /* 1/1024 -> 1 */;			double zDt[8] = { Arf.msTime * 1024.0 };
	AuInfo info = { .frameDt = (uint64_t)(lua_tonumber(L, 2) * 1000) };
	Delta timer = { .t = Arf.msTime >> 2 };

	/* Delta
	 * zDt = Scale * 1024, Dt = Scale * Speed
	 */
	if( auto pFirst = Arf.deltas.begin() + 1; true ) {   // To limit the scope of some vars
		for( uint64_t sizes = Arf.deltas[0].val,  i = 7; i; --i,  sizes >>= 9 )
			if( const uint64_t size = sizes & 0x1ff;  size == 0 )
				zDt[i] = zDt[0];
			else {
				const auto pEnd  = pFirst + size,
						   pNext = std::upper_bound(pFirst, pEnd, timer, dtPred);
				if( const Delta thisDt = *(pNext-1);  pNext != pEnd  &&  pNext->base < thisDt.base )
					zDt[i] = thisDt.base - (Arf.msTime - thisDt.t * 4.0) * thisDt.absV;
				else
					zDt[i] = thisDt.base + (Arf.msTime - thisDt.t * 4.0) * thisDt.absV;
				pFirst = pEnd;
			}
		const auto pEnd  = Arf.deltas.end(),
				   pNext = std::upper_bound(pFirst, pEnd, timer, dtPred);
		if( const Delta thisDt = *(pNext-1);  pNext != pEnd  &&  pNext->base < thisDt.base )
			zDt[0] = thisDt.base - (Arf.msTime - thisDt.t * 4.0) * thisDt.absV;
		else
			zDt[0] = thisDt.base + (Arf.msTime - thisDt.t * 4.0) * thisDt.absV;
		timer.t >>= 7;
	}

	/* Wish */
	lastWgo.clear();		   // timer.t == Arf.msTime >> 9 since here
	for(const Info wi = Arf.wIdx[timer.t];  Wish& wish : Span(Arf.wishes.begin() + wi.f, wi.c)) {
		Wish w = wish;

		/* Nodes */
		const auto nodes = Span(Arf.nodes.begin() + w.nSince, w.nCount);
		if( Arf.msTime < nodes.front().ms  ||  Arf.msTime >= nodes.back().ms )
			continue;

		Point thiz, next;
		if( thiz = nodes[w.nIndex], Arf.msTime < thiz.ms )
			do	 --w.nIndex;
			while( thiz = nodes[w.nIndex], Arf.msTime < thiz.ms );
		else if( uint8_t nextIdx = w.nIndex + 1;  next = nodes[nextIdx],  Arf.msTime >= next.ms )
			do	 ++w.nIndex, ++nextIdx;
			while( next = nodes[nextIdx], Arf.msTime >= next.ms );
		info.sType = w.isSpecial;

		const float tint = (Arf.msTime - nodes[0].ms) / 151.0f,
					ratio = Eased( (double)(Arf.msTime - thiz.ms) / (next.ms - thiz.ms), thiz.ease ),
					radius = (thiz.radius + (next.radius - thiz.radius) * ratio) * 2; /* 1/4 -> 1/8 */
		Duo nodePos = CosSin({ .a = thiz.deg + (next.deg - thiz.deg) * ratio });
			nodePos.a = thiz.cdx + (next.cdx - thiz.cdx) * ratio + radius * nodePos.a /* cos(deg) */ ;
			nodePos.b = thiz.cdy + (next.cdy - thiz.cdy) * ratio + radius * nodePos.b /* sin(deg) */ ;
		info = renderWish(L, info, nodePos, {.a = 0.01f, .b = max(tint, 1.0f) });

		/* WishChild */
		if( double wZdt;  w.cCount )
			if( const auto wChilds = Span(Arf.wishChilds.begin() + w.cSince, w.cCount);
				(wZdt = zDt[w.delGroup]) < wChilds.back().zDt  &&  (wChilds[0].zDt - wZdt) * dSpeed < 8 ) {

				// Manage cIndex
				if( uint16_t prevCidx = w.cIndex - 1;  w.cIndex  &&  wZdt < wChilds[prevCidx].zDt )
					do	 --w.cIndex, --prevCidx;
					while( w.cIndex  &&  wZdt < wChilds[prevCidx].zDt );
				else {
					const uint16_t lastCidx = w.cCount - 1;
					while( w.cIndex < lastCidx  &&  wZdt >= wChilds[w.cIndex].zDt )
						++w.cIndex;
				}

				// Traverse Subspan
				for(const auto child : wChilds.subspan(w.cIndex)) {
					const auto distX8 = (child.zDt - wZdt) * dSpeed * 8;
					if( distX8 > 64 /* 8x8 */ )
						break;

					const double cRatio = 1.0 - distX8 / (child.radius << 1);   /* 1/4 -> 1/8 */
					if( cRatio <= 0 )
						continue;

					Duo childPos = CosSin({
						.a = (float)( 360 * (child.initLoop / 64.0 + child.deltaLoop / 8.0 * cRatio) )
					});
					childPos.a = nodePos.a + distX8 * childPos.a;
					childPos.b = nodePos.b + distX8 * childPos.b;

					const float cTint = cRatio / 0.237;
					info = renderWish(L, info, childPos, { .a = 0.03f, .b = max(cTint, 1.0f) });
				}
			}
		wish = w;   // `w` is a value, while `wish` is a ref
	}

	/* Hint & Echo */
	if( Arf.isAuto ) {   // There are much more boilerplate lines...
		for( const Info hi = Arf.hIdx[timer.t];  const Hint h : Span(Arf.hints.begin() + hi.f, hi.c) ) {
			const int16_t lifeMs = Arf.msTime - h.ms;
			if( lifeMs > +370 )		continue;   // +470 if not Auto
			if( lifeMs < -510 )		break;

			const Duo hintPos = { .a = 900 + h.cdx * Arf.xScale + Arf.xDelta,
								  .b = 540 + h.cdy * Arf.yScale };
			const GO hintGo   = ( lua_rawgeti(L, HGO,   ++info.hUsed), dmScript::CheckGOInstance(L,-1) );
			const v4 hintTint = ( lua_rawgeti(L, HTINT, info.hUsed--), dmScript::CheckVector4(L,-1)    );
			lua_pop(L, 2);

			if( float V;  lifeMs < -370 )
				V = lifeMs * 0.0001 - 0.037,				SetPosition( hintGo, P3(hintPos.a, hintPos.b, V) ),
				V = 0.3f + (lifeMs + 510) * 0.0005f,		hintTint -> setX(V).setY(V).setZ(V),
				info.hUsed++;
			else if( lifeMs < 0 )
				hintTint -> setX(0.37).setY(0.37).setZ(0.37),
				SetPosition( hintGo, P3(hintPos.a, hintPos.b, -0.0573) ),
				info.hUsed++;
			else
				info = renderAnim(L, info, hintPos, lifeMs),
				( lifeMs < 101 )?
					SetPosition( hintGo, P3(hintPos.a, hintPos.b, -0.0073) ),
					hintTint -> setXYZ(Arf.hTint) : 0,
				info.playH = lifeMs < info.frameDt;
		}
		for( const Info ei = Arf.eIdx[timer.t];  const Echo e : Span(Arf.echoes.begin() + ei.f, ei.c) ) {

		}
	}
	else {
		for( const Info hi = Arf.hIdx[timer.t];  const Hint h : Span(Arf.hints.begin() + hi.f, hi.c) ) {
			const int16_t lifeMs = Arf.msTime - h.ms;
			if( lifeMs > +470 )		continue;
			if( lifeMs < -510 )		break;

			const Duo hintPos = { .a = 900 + h.cdx * Arf.xScale + Arf.xDelta,
								  .b = 540 + h.cdy * Arf.yScale };
			const GO hintGo   = ( lua_rawgeti(L, HGO,   ++info.hUsed), dmScript::CheckGOInstance(L,-1) );
			const v4 hintTint = ( lua_rawgeti(L, HTINT, info.hUsed--), dmScript::CheckVector4(L,-1)    );
			lua_pop(L, 2);

			if( float V;  lifeMs < -370 )
				V = lifeMs * 0.0001 - 0.037,				SetPosition( hintGo, P3(hintPos.a, hintPos.b, V) ),
				V = 0.3f + (lifeMs + 510) * 0.0005f,		hintTint -> setX(V).setY(V).setZ(V),
				info.hUsed++;
			else if( lifeMs < 370 ) switch( h.status ) {
				case NJUDGED:		case SPECIAL:
					hintTint -> setX(0.37).setY(0.37).setZ(0.37);
					dmGameObject::SetPosition( hintGo, P3(hintPos.a, hintPos.b, -0.0637) );
					info.hUsed++;
					break;
				case NJUDGED_LIT:	case SPECIAL_LIT:
					hintTint -> setX(0.573).setY(0.573).setZ(0.573);
					dmGameObject::SetPosition( hintGo, P3(hintPos.a, hintPos.b, -0.0573) );
					info.hUsed++;
					break;
				case HIT_LIT:
					hintTint -> setXYZ(Arf.hTint);
					dmGameObject::SetPosition( hintGo, P3(hintPos.a, hintPos.b, -0.0073) );
					info.hUsed++;
				case HIT:
					info.sType = 0, info = renderAnim(L, info, hintPos, lifeMs - h.deltaMs);
					break;
				case EARLY_LIT:
					hintTint -> setXYZ(HintEarly);
					dmGameObject::SetPosition( hintGo, P3(hintPos.a, hintPos.b, -0.0037) );
					info.hUsed++;
				case EARLY:
					info.sType = 1, info = renderAnim(L, info, hintPos, lifeMs - h.deltaMs);
					break;
				case LATE_LIT:		HCASE_LATE_LIT:;
					hintTint -> setXYZ(HintLate);
					dmGameObject::SetPosition( hintGo, P3(hintPos.a, hintPos.b, -0.0037) );
					info.hUsed++;
				case LATE:			HCASE_LATE:;
					info.sType = 2, info = renderAnim(L, info, hintPos, lifeMs - h.deltaMs);
					break;
				default:   // LOST
					SetPosition( hintGo, P3(hintPos.a, hintPos.b, -lifeMs * 0.00011) );
					V =  0.573 - lifeMs * 0.00037,		hintTint -> setX(V);
					V *= 0.51,							hintTint -> setY(V).setZ(V);
					info.hUsed++;
			}
			else if( h.deltaMs > 0 ) switch( h.status ) {   // `goto` is used to reduce boilerplate lines
				case LATE_LIT:		goto HCASE_LATE_LIT;
				case LATE:			goto HCASE_LATE;
				default:;
			}
		}
		for( const Info ei = Arf.eIdx[timer.t];  const Echo e : Span(Arf.echoes.begin() + ei.f, ei.c) ) {

		}
	}

	/* Do Returns */
	return lua_pushinteger(L, info.wUsed), lua_pushinteger(L, info.hUsed), lua_pushinteger(L, info.eUsed),
		   lua_pushinteger(L, info.xUsed), lua_pushinteger(L, info.aUsed), lua_pushboolean(L, info.playH),
		   lua_pushboolean(L, info.playE), 7;
}