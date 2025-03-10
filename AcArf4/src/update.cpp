//  Arf4 Update  //
#include <Arf4.h>
#include <dmsdk/dlib/time.h>
#include <unordered_map>
#include <algorithm>
#include <span>

static constexpr auto H_EARLY_R = 0.37675f, H_EARLY_G = 0.67815f, H_EARLY_B = 0.767628125f;
static constexpr auto H_LATE_R = 0.767628125f, H_LATE_G = 0.466228125f, H_LATE_B = 0.37675f;
static constexpr auto H_HIT_R = 0.88f, H_HIT_G = 0.7528125f, H_HIT_B = 0.5534375f;

static constexpr auto A_EARLY_R = 0.3125f, A_EARLY_G = 0.5625f, A_EARLY_B = 0.63671875f;
static constexpr auto A_LATE_R = 0.63671875f, A_LATE_G = 0.38671875f, A_LATE_B = 0.3125f;
static constexpr auto A_HIT_R = 1.0f, A_HIT_G = 0.85546875f, A_HIT_B = 0.62890625f;

typedef dmGameObject::HInstance GO;				using namespace Ar;
typedef dmVMath::Vector3 v3i, *v3;				typedef dmVMath::Point3 p3;
typedef dmVMath::Vector4 v4i, *v4;				typedef dmVMath::Quat Qt;

struct AuInfo {
	uint64_t frameDt:11 = 0, aUsed:11 = 0, wUsed:10 = 0, eUsed:10 = 0, xUsed:10 = 0, hUsed:9 = 0;
	uint64_t playH:1 = false, playE:1 = false, wishSpecial = false;
};


/* Utils & Render Methods */
static const Qt maxQuat(0.0f, 0.0f, 0.594822786751341f, 0.803856860617217f);
static Qt rotationToQuat(const float degree) noexcept {
	const auto cosSin = CosSin({ .a = degree * 0.5f });
	return Qt(0.0f, 0.0f, cosSin.b, cosSin.a);
}

static std::unordered_map<uint64_t, int16_t> lastWgo;
static AuInfo renderWish(lua_State* L, AuInfo info, Duo Pos, Duo zw) {
	if( Pos.b = 540 + Pos.b * Arf.yScale,  Pos.b >= -36  &&  Pos.b <= 1116 )
		if( Pos.a = 900 + Pos.a * Arf.xScale + Arf.xDelta,  Pos.a >= -36  &&  Pos.a <= 1836 )
			if( const uint16_t idx = lastWgo[Pos.val];  idx == 0 ) {
				lastWgo[Pos.val] = ++info.wUsed;
				const auto wGo = ( lua_rawgeti(L, WGO, info.wUsed),
										   dmScript::CheckGOInstance(L, -1) );
				SetPosition( wGo, p3(Pos.a, Pos.b, zw.a) );
				SetScale   ( wGo, (zw.b = 1 - zw.b,  0.637f + 0.437f * zw.b * zw.b) );

				// Tint
				lua_pushnumber(L, info.wishSpecial ? -zw.b : zw.b), lua_rawseti(L, WTINT, info.wUsed);
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
					lua_pushnumber(L, info.wishSpecial ? -1 : 1), lua_rawseti(L, WTINT, idx);
				lua_pop(L, 2);
			}
	return info;
}

static AuInfo renderHint(AuInfo info) {
	return info;
}

static AuInfo renderEcho(AuInfo info) {
	return info;
}

static AuInfo renderAnim(AuInfo info) {
	return info;
}

static constexpr auto dtPred = [](const Delta a, const Delta b) noexcept {
	return a.t < b.t;
};


/* Main */
int Ar::UpdateArf(lua_State* L) noexcept {
	/* Usage:
	 * local wgo_used, hgo_used, ego_used, ehgo_used, ago_used, h_playhs, e_playhs = Arf4.UpdateArf(
	 *       ms, dt, wgos, hgos, egos, ehgos, agos, wtints, htints, etints, ehtints, atints)
	 */
	Arf.msTime = lua_tointeger(L, 1);
		if( Arf.msTime >= Arf.before )			return 0;
			Arf.msTime = max(Arf.msTime, 2);	UsysTime = dmTime::GetMonotonicTime();
	#ifndef AR_BUILD_VIEWER
		if (! Arf.isAuto )						JudgeArfSweep();
	#endif

	/* Info */
	const double eSpeed = (PlayerSpeed * Arf.cSpeed + 11) / 1500.0, echoDt = Arf.msTime * eSpeed,
				 dSpeed = eSpeed / 1024 /* 1/1024 -> 1 */;			double oDt[8] = { Arf.msTime * 1024.0 };
	AuInfo info = { .frameDt = (uint64_t)(lua_tonumber(L, 2) * 1000) };
	Delta timer = { .t = Arf.msTime >> 2 };

	/* Delta */
	if( auto pFirst = Arf.deltas.begin() + 1; true ) {   // To limit the scope of some vars
		for( uint64_t sizes = Arf.deltas[0].val,  i = 7; i; --i,  sizes >>= 9 )
			if( const uint64_t size = sizes & 0x1ff;  size == 0 )
				oDt[i] = oDt[0];
			else {
				const auto pEnd  = pFirst + size,
						   pNext = std::upper_bound(pFirst, pEnd, timer, dtPred);
				if( const Delta thisDt = *(pNext-1);  pNext != pEnd  &&  pNext->base < thisDt.base )
					oDt[i] = thisDt.base - (Arf.msTime - thisDt.t * 4.0) * thisDt.absV;
				else
					oDt[i] = thisDt.base + (Arf.msTime - thisDt.t * 4.0) * thisDt.absV;
				pFirst = pEnd;
			}
		const auto pEnd  = Arf.deltas.end(),
				   pNext = std::upper_bound(pFirst, pEnd, timer, dtPred);
		if( const Delta thisDt = *(pNext-1);  pNext != pEnd  &&  pNext->base < thisDt.base )
			oDt[0] = thisDt.base - (Arf.msTime - thisDt.t * 4.0) * thisDt.absV;
		else
			oDt[0] = thisDt.base + (Arf.msTime - thisDt.t * 4.0) * thisDt.absV;
		timer.t >>= 7;
	}

	/* Wish */
	lastWgo.clear();		   // timer.t == Arf.msTime >> 9 since here
	for(const Info wi = Arf.wIdx[timer.t];  Wish& wish : std::span(Arf.wishes.begin() + wi.f, wi.c)) {
		Wish w = wish;

		/* Nodes */
		const auto nodes = std::span(Arf.nodes.begin() + w.nSince, w.nCount);
		if( Arf.msTime < nodes.front().ms  ||  Arf.msTime >= nodes.back().ms )
			continue;

		Point thiz, next;
		if( thiz = nodes[w.nIndex], Arf.msTime < thiz.ms )
			do	 --w.nIndex;
			while( thiz = nodes[w.nIndex], Arf.msTime < thiz.ms );
		else if( uint8_t nextIdx = w.nIndex + 1;  next = nodes[nextIdx],  Arf.msTime >= next.ms )
			do	 ++w.nIndex, ++nextIdx;
			while( next = nodes[nextIdx], Arf.msTime >= next.ms );
		info.wishSpecial = w.isSpecial;

		const float tint = (Arf.msTime - nodes[0].ms) / 151.0f,
					ratio = Eased( (double)(Arf.msTime - thiz.ms) / (next.ms - thiz.ms), thiz.ease ),
					radius = (thiz.radius + (next.radius - thiz.radius) * ratio) * 2; /* 1/4 -> 1/8 */
		Duo nodePos = CosSin({ .a = thiz.deg + (next.deg - thiz.deg) * ratio });
			nodePos.a = thiz.cdx + (next.cdx - thiz.cdx) * ratio + radius * nodePos.a /* cos(deg) */ ;
			nodePos.b = thiz.cdy + (next.cdy - thiz.cdy) * ratio + radius * nodePos.b /* sin(deg) */ ;
		info = renderWish(L, info, nodePos, {.a = 0.01f, .b = max(tint, 1.0f) });

		/* WishChild */
		if( double wOdt;  w.cCount )
			if(const auto wChilds = std::span(Arf.wishChilds.begin() + w.cSince, w.cCount);
				(wOdt = oDt[w.deltaGroup]) < wChilds.back().oDt  &&  (wChilds[0].oDt - wOdt) * dSpeed < 8) {

				// Manage cIndex
				if( uint16_t prevCidx = w.cIndex - 1;  w.cIndex  &&  wOdt < wChilds[prevCidx].oDt )
					do	 --w.cIndex, --prevCidx;
					while( w.cIndex  &&  wOdt < wChilds[prevCidx].oDt );
				else {
					const uint16_t lastCidx = w.cCount - 1;
					while( w.cIndex < lastCidx  &&  wOdt >= wChilds[w.cIndex].oDt )
						++w.cIndex;
				}

				// Traverse Subspan
				for(const auto child : wChilds.subspan(w.cIndex)) {
					const auto distX8 = (child.oDt - wOdt) * dSpeed * 8;
					if( distX8 > 64 /* 8x8 */ )
						break;

					const double cRatio = 1.0 - distX8 / (child.radius << 1);   /* 1/4 -> 1/8 */
					if( cRatio <= 0 )
						continue;

					Duo childPos = CosSin({
						.a = 360 * (float)(child.initLoop / 64.0 + child.deltaLoop / 8.0 * cRatio)
					});
					childPos.a = nodePos.a + distX8 * childPos.a;
					childPos.b = nodePos.b + distX8 * childPos.b;

					const float cTint = ratio / 0.237;
					info = renderWish(L, info, childPos, { .a = 0.03f, .b = max(cTint, 1.0f) });
				}
			}
		wish = w;   // `w` is a value, while `wish` is a ref
	}

	/* Hint */
	for( const Info hi = Arf.hIdx[timer.t];  Hint& hint : std::span(Arf.hints.begin() + hi.f, hi.c) ) {

	}

	/* Echo */
	for( const Info ei = Arf.eIdx[timer.t];  Echo& echo : std::span(Arf.echoes.begin() + ei.f, ei.c) ) {

	}
	return lua_pushinteger(L, info.wUsed), lua_pushinteger(L, info.hUsed), lua_pushinteger(L, info.eUsed),
		   lua_pushinteger(L, info.xUsed), lua_pushinteger(L, info.aUsed), lua_pushboolean(L, info.playH),
		   lua_pushboolean(L, info.playE), 7;
}