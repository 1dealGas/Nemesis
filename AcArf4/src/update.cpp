//  Arf4 Update  //
#include <Arf4.h>
#include <dmsdk/dlib/time.h>
#include <span>

static const dmVMath::Vector3
	AnimTint[]{ {1, 1, 1}, {0.3125, 0.5625, 0.63671875}, {0.63671875, 0.38671875, 0.3125} },
	HintEarly	{0.37675, 0.67815, 0.767628125},
	HintLate	{0.767628125, 0.466228125, 0.37675},
	HintHit		{0.88, 0.88, 0.88};

typedef dmGameObject::HInstance GO;				using namespace Ar;
typedef dmVMath::Vector3 v3i, *v3;				typedef dmVMath::Point3 P3;
typedef dmVMath::Vector4 v4i, *v4;				typedef dmVMath::Quat Qt;

struct AuInfo {
	uint64_t frameDt:6 = 0, wUsed:10 = 0, aUsed:10 = 0, eUsed:9 = 0, xUsed:9 = 0, hUsed:9 = 0;
	uint64_t sType:2 = false, playH:1 = false, playE:1 = false;
};

/* Utils & Render Methods */
static const Qt maxQuat(0, 0, 0.594822786751341, 0.803856860617217);
static auto rotationToQuat(const float degree) noexcept {
	const auto cosSin = CosSin({ .a = degree * 0.5f });
	return Qt(0, 0, cosSin.b, cosSin.a);
}

static AuInfo renderWish(lua_State* L, AuInfo info, Duo Pos, const Duo zw) {
	if( Pos.b = 540 + Pos.b * Arf.yScale,  Pos.b >= -36  &&  Pos.b <= 1116 )
		if( Pos.a = 900 + Pos.a * Arf.xScale + Arf.xDelta,  Pos.a >= -36  &&  Pos.a <= 1836 ) {
			const auto wGo = ( lua_rawgeti(L, WGO, ++info.wUsed),
							   dmScript::CheckGOInstance(L,-1) );
			// Tint
			lua_pushnumber(L, info.sType ? -zw.b : zw.b);
			lua_rawseti(L, WTINT, info.wUsed);

			// Transform
			SetPosition( wGo, P3(Pos.a, Pos.b, zw.a) );
			SetScale   ( wGo, 1.074 - 0.437 * zw.b * (2-zw.b) );   // Scale: 1.074 -> 0.637
			lua_pop(L, 1);
		}
	return info;
}

static AuInfo renderAnim(lua_State* L, AuInfo info, const Duo Pos, const int16_t msPast) {
	if( msPast > 370 )	return info;
	const auto tint = ( lua_rawgeti(L, ATINT, ++info.aUsed), dmScript::CheckVector4(L, -1) );
	const auto lAgo = ( lua_rawgeti(L, AL, info.aUsed), dmScript::CheckGOInstance(L, -1) ),
			   rAgo = ( lua_rawgeti(L, AR, info.aUsed), dmScript::CheckGOInstance(L, -1) );
	lua_pop(L, 3);

	// Tint
	tint -> setXYZ( AnimTint[info.sType] );
	if( double w;  msPast < 73 )
		w = (msPast + 27) * 0.01,		tint -> setW( 0.637 * w * (2-w) );
	else
		w = (370-msPast) / 297.0,		tint -> setW( 0.637 * w * w );

	// Transform
	const float PosZ = msPast * 0.0001;
	SetPosition(lAgo, P3( Pos.a, Pos.b, PosZ ));
	SetPosition(rAgo, P3( Pos.a, Pos.b, PosZ + 0.00005 ));

	if( double leftRatio;  msPast < 193 )
		leftRatio = msPast / 193.0,
		SetRotation( lAgo, rotationToQuat(45 + 28 * leftRatio) ),
		SetScale( lAgo, 1 + 0.637 * leftRatio * (2 - leftRatio) );
	else
		SetRotation(lAgo, maxQuat),
		SetScale(lAgo, 1.637);

	const double rightRatio = msPast / 370.0;
	SetRotation( rAgo, rotationToQuat(45 - 8 * rightRatio) );
	SetScale( rAgo, 1 + 0.637 * rightRatio * (2 - rightRatio) );
	return info;
}


/* Main */
int Ar::UpdateArf(lua_State* L) noexcept {
	/* Usage:
	 * local wgo_used, hgo_used, ego_used, ehgo_used, ago_used, h_plhs, e_plhs = Arf4.UpdateArf(
	 *       ms, wgos, hgos, egos, ehgos, agols, agors, wtints, htints, etints, ehtints, atints)
	 */
	UsysTime = dmTime::GetMonotonicTime();

	AuInfo info;
	if( int32_t lastMs = Arf.msTime;  Arf.msTime = lua_tointeger(L, 1),  Arf.msTime >= Arf.before )
		return 0;
	else if( lastMs = Arf.msTime - lastMs,  lastMs > 0 )
		info.frameDt = lastMs > 33 ? 34 : lastMs;
	#ifndef AR_BUILD_VIEWER
		if(! Arf.isAuto )						JudgeArfSweep();
	#endif

	const double eSpeed = (PlayerSpeed * Arf.cSpeed + 11) / 1500.0;	  double zDt[2] = {Arf.msTime * 1024.0};
	const double dSpeed = eSpeed / 1024 /* 1/1024 -> 1 */;				auto zTimer = (Arf.msTime >> 2);

	/* Delta
	 * zDt = Scale * 1024, Dt = Scale * xSpeed
	 */
	if( const Delta lastDt = Arf.deltas.back();  zTimer >= lastDt.t )
		zDt[1] = lastDt.base + (Arf.msTime - lastDt.t * 4.0) * lastDt.absV;
	else {
		const auto initIt = Arf.deltas.begin() + 1, lastIt = Arf.deltas.end() - 1;
			  auto it = initIt + Arf.deltas[0].val;
		while( it != initIt  &&  zTimer < it->t ) { --it; }
		/**/auto nextIt = it + 1;
		while( it != lastIt  &&  zTimer >= nextIt->t )
			++it, ++nextIt;

		if( const Delta thiz = *it;  thiz.base <= nextIt->base )
			zDt[1] = thiz.base + (Arf.msTime - thiz.t * 4.0) * thiz.absV;
		else
			zDt[1] = thiz.base - (Arf.msTime - thiz.t * 4.0) * thiz.absV;
		Arf.deltas[0].val = it - initIt;
	}
	zTimer >>= 8;   // zTimer == Arf.msTime >> 10 since here

	/* Wish */
	for(const Info wi = Arf.wIdx[zTimer >> 1];  Wish& wish : std::span(Arf.wishes).subspan(wi.f, wi.c)) {
		Wish w = wish;

		/* Nodes */
		const auto nodes = std::span(Arf.nodes).subspan(w.nSince, w.nCount);
		if( Arf.msTime < nodes.front().ms  ||  Arf.msTime >= nodes.back().ms )
			continue;

		Point thiz, next;
		if( thiz = nodes[w.nIndex], Arf.msTime < thiz.ms ) {
			do	 --w.nIndex;
			while( thiz = nodes[w.nIndex], Arf.msTime < thiz.ms );
			next = nodes[w.nIndex + 1];
		}
		else if( uint8_t nextIdx = w.nIndex + 1;  next = nodes[nextIdx],  Arf.msTime >= next.ms ) {
			do	 ++w.nIndex, ++nextIdx;
			while( next = nodes[nextIdx], Arf.msTime >= next.ms );
			thiz = nodes[w.nIndex];
		}
		info.sType = w.isSpecial;

		const float tint = (Arf.msTime - nodes[0].ms) / 151.0,
					ratio = Eased( (double)(Arf.msTime - thiz.ms) / (next.ms - thiz.ms), thiz.ease ),
					radius = (thiz.radius + (next.radius - thiz.radius) * ratio) * 2; /* 1/4 -> 1/8 */
		Duo nodePos   = CosSin({ .a = thiz.deg + (next.deg - thiz.deg) * ratio });
			nodePos.a = thiz.cdx + (next.cdx - thiz.cdx) * ratio + radius * nodePos.a /* cos(deg) */ ;
			nodePos.b = thiz.cdy + (next.cdy - thiz.cdy) * ratio + radius * nodePos.b /* sin(deg) */ ;
		info = renderWish(L, info, nodePos, { .a = 0.01f, .b = (float)fmin(tint, 1.0f) });

		/* WishChild */
		if( double wZdt;  w.cCount )
			if( const auto wChilds = std::span(Arf.wishChilds).subspan(w.cSince, w.cCount);
				(wZdt = zDt[w.withDt]) < wChilds.back().zDt  &&  (wChilds[0].zDt - wZdt) * dSpeed < 8 ) {

				// Manage cIndex
				if( uint16_t prevCidx = w.cIndex - 1;  w.cIndex  &&  wZdt < wChilds[prevCidx].zDt )
					do	 --w.cIndex, --prevCidx;
					while( w.cIndex  &&  wZdt < wChilds[prevCidx].zDt );
				else if( const uint16_t lastCidx = w.cCount - 1;  true )
					while(w.cIndex < lastCidx  &&  wZdt >= wChilds[w.cIndex].zDt)
						++w.cIndex;

				// Traverse Subspan
				for( const auto c : wChilds.subspan(w.cIndex) )
					if( double cQuot = 1 - (c.zDt - wZdt) * dSpeed / fmax(c.radius / 4.0, 6);  cQuot > 0 ) {
						Duo childPos = CosSin({
							.a = (float)( 360 * (c.initLoop / 64.0 + c.deltaLoop / 8.0 * cQuot) )
						});
						const float cTint = cQuot / 0.237;
									cQuot = (1-cQuot) * (c.radius << 1);   /* 1/4 -> 1/8 */
						childPos.a = nodePos.a + cQuot * childPos.a;
						childPos.b = nodePos.b + cQuot * childPos.b;
						info = renderWish(L, info, childPos, { .a = 0.03f, .b = (float)fmin(cTint, 1.0f) });
					}
			}
		wish = w;   // `w` is a value, while `wish` is a ref
	}

	/* Hint & Echo */
	if( info.sType = 0,  Arf.isAuto ) {   // There are much more boilerplate lines...
		for(const Info hi = Arf.hIdx[zTimer];  const Hint h : std::span(Arf.hints).subspan(hi.f, hi.c)) {
			const int16_t lifeMs = Arf.msTime - h.ms;
			if( lifeMs > +370 )		continue;   // +470 if not Auto
			if( lifeMs < -510 )		break;

			const Duo hintPos  = { .a = 900 + h.cdx * Arf.xScale + Arf.xDelta,
								   .b = 540 + h.cdy * Arf.yScale };
			const GO  hintGo   = ( lua_rawgeti(L, HGO, ++info.hUsed), dmScript::CheckGOInstance(L,-1) );
			const v4  hintTint = ( lua_rawgeti(L, HTINT, info.hUsed), dmScript::CheckVector4(L,-1)    );
			lua_pop(L, 2);

			if( float V;  lifeMs < -370 )
				V = lifeMs * 0.0001 - 0.037,			SetPosition( hintGo, P3(hintPos.a, hintPos.b, V) ),
				V = 0.3 + (lifeMs + 510) * 0.0005,		hintTint -> setX(V).setY(V).setZ(V);
			else if( lifeMs < 0 )
				SetPosition( hintGo, P3(hintPos.a, hintPos.b, -0.0573) ),
				hintTint -> setX(0.37).setY(0.37).setZ(0.37);
			else
				info = renderAnim(L, info, hintPos, lifeMs),
				( lifeMs < 101 )?
					SetPosition( hintGo, P3(hintPos.a, hintPos.b, -0.0073) ), hintTint -> setXYZ(HintHit), 0
					:--info.hUsed,   // Hint Go acquired, but not used
				info.playH = lifeMs < info.frameDt;
		}
		for(const Info ei = Arf.eIdx[zTimer];  const Echo e : std::span(Arf.echoes).subspan(ei.f, ei.c)) {
			const int16_t lifeMs = Arf.msTime - e.ms;
			if( lifeMs > 370 )
				continue;

			Duo ePos, mPos = { .a = 900 + e.cdx * Arf.xScale + Arf.xDelta, .b = 540 + e.cdy * Arf.yScale };
			double R = 1;

			if( lifeMs >= 0 )
				ePos = mPos;
			else if( !e.radius )
				if( lifeMs > -638 )		ePos = mPos, R = (lifeMs + 637) / 151.0;
				else					continue;
			else if( double x8d;  R += lifeMs * eSpeed / fmax(e.radius * 0.25, 6),  R < 0 )
				goto MISC_UPDATE_AUTO;
			else
				x8d = (1-R) * (e.radius << 1),   /* 1/4 -> 1/8 */
				ePos.a = 360 * (e.initLoop / 64.0 + e.deltaLoop / 8.0 * R),  ePos = CosSin(ePos),
				ePos.a = mPos.a + x8d * ePos.a * Arf.xScale,
				ePos.b = mPos.b + x8d * ePos.b * Arf.yScale,
				R /= 0.237;
			R = fmin( R,1 );

			// Update Echo
			if( lifeMs < 101 ) {
				const GO echoGo   = ( lua_rawgeti(L, EGO, ++info.eUsed), dmScript::CheckGOInstance(L,-1) );
				const v4 echoTint = ( lua_rawgeti(L, ETINT, info.eUsed), dmScript::CheckVector4(L,-1)    );
				lua_pop(L, 2);

				if( lifeMs < -510 ) {
					echoTint -> setX(0.3).setY(0.3).setZ(0.3).setW(R);
					goto E_ASET_SCL;
				}
				if( lifeMs < -370 ) {
					const float C = 0.0005 * (lifeMs + 370) + 0.37;
					echoTint -> setX(C).setY(C).setZ(C).setW(R);
					goto E_ASET_SCL;
				}
				if( lifeMs < 0 ) {
					echoTint -> setX(0.37).setY(0.37).setZ(0.37).setW(R);
	E_ASET_SCL:		SetScale( echoGo, 1.074 - 0.437 * R * (2-R) );
				}
				else
					echoTint -> setXYZ(HintHit).setW(R),
					SetScale( echoGo, 0.637 );
				SetPosition( echoGo, P3(ePos.a, ePos.b, 0.02) );
			}

			/**/ MISC_UPDATE_AUTO:
			if( lifeMs > 0 )
				info = renderAnim(L, info, mPos, lifeMs),
				info.playE = (lifeMs < info.frameDt)  &&  e.status;
			else if( lifeMs > -511 ) {
				const GO helper = ( lua_rawgeti(L, EH, ++info.xUsed), dmScript::CheckGOInstance(L,-1) );
				lua_pushnumber( L, R = 1 + lifeMs / 510.0 ), lua_rawseti(L, EHTINT, info.xUsed);
				SetPosition( helper, P3(mPos.a, mPos.b, 0.0625) );
				SetScale( helper, 1.237 - R * (2-R) );
				lua_pop(L, 1);
			}
		}
	}
#ifndef AR_BUILD_VIEWER
	else {
		for(const Info hi = Arf.hIdx[zTimer];  const Hint h : std::span(Arf.hints).subspan(hi.f, hi.c)) {
			const int16_t lifeMs = Arf.msTime - h.ms;
			if( lifeMs > +470 )		continue;
			if( lifeMs < -510 )		break;

			const Duo hintPos  = { .a = 900 + h.cdx * Arf.xScale + Arf.xDelta,
								   .b = 540 + h.cdy * Arf.yScale };
			const GO  hintGo   = ( lua_rawgeti(L, HGO,   ++info.hUsed), dmScript::CheckGOInstance(L,-1) );
			const v4  hintTint = ( lua_rawgeti(L, HTINT, info.hUsed--), dmScript::CheckVector4(L,-1)    );
			lua_pop(L, 2);

			if( float V;  lifeMs < -370 )
				V = lifeMs * 0.0001 - 0.037,			SetPosition( hintGo, P3(hintPos.a, hintPos.b, V) ),
				V = 0.3 + (lifeMs + 510) * 0.0005,		hintTint -> setX(V).setY(V).setZ(V),  ++info.hUsed;
			else if( lifeMs < 370 ) switch( h.status ) {
	[[likely]]	case NJUDGED:		case SPECIAL:
					hintTint -> setX(0.37).setY(0.37).setZ(0.37);
					SetPosition( hintGo, P3(hintPos.a, hintPos.b, -0.0637) ),  ++info.hUsed;
					break;
				case NJUDGED_LIT:	case SPECIAL_LIT:
					hintTint -> setX(0.573).setY(0.573).setZ(0.573);
					SetPosition( hintGo, P3(hintPos.a, hintPos.b, -0.0573) ),  ++info.hUsed;
					break;
				case HIT_LIT:
					hintTint -> setXYZ(HintHit);
					SetPosition( hintGo, P3(hintPos.a, hintPos.b, -0.0073) ),  ++info.hUsed;
				case HIT:
					info = renderAnim(L, info, hintPos, lifeMs - h.deltaMs);
					break;
  [[unlikely]]  case EARLY_LIT:
					hintTint -> setXYZ(HintEarly);
					SetPosition( hintGo, P3(hintPos.a, hintPos.b, -0.0037) ),  ++info.hUsed;
  [[unlikely]]  case EARLY:
					info.sType = 1, info = renderAnim(L, info, hintPos, lifeMs - h.deltaMs), info.sType = 0;
					break;
  [[unlikely]]  case LATE_LIT:		/**/ HCASE_LATE_LIT:
					hintTint -> setXYZ(HintLate);
					SetPosition( hintGo, P3(hintPos.a, hintPos.b, -0.0037) ),  ++info.hUsed;
  [[unlikely]]  case LATE:			/**/ HCASE_LATE:
					info.sType = 2, info = renderAnim(L, info, hintPos, lifeMs - h.deltaMs), info.sType = 0;
					break;
  [[unlikely]]  default:   // LOST
					SetPosition( hintGo, P3(hintPos.a, hintPos.b, -lifeMs * 0.00011) ),  ++info.hUsed;
					V = 0.573 - lifeMs * 0.00037,	hintTint -> setX(V);
					V*= 0.51,						hintTint -> setY(V).setZ(V);
			}
			else if( h.deltaMs > 0 ) switch( h.status ) {   // `goto` is used to reduce boilerplate lines
  [[unlikely]]  case LATE_LIT:		goto HCASE_LATE_LIT;
  [[unlikely]]  case LATE:			goto HCASE_LATE;
				default:;
			}
		}
		for(const Info ei = Arf.eIdx[zTimer];  const Echo e : std::span(Arf.echoes).subspan(ei.f, ei.c)) {
			const int16_t lifeMs = Arf.msTime - e.ms;
			if( lifeMs > 470 )
				continue;

			double R = 1;
			Duo ePos, mPos = { .a = 900 + e.cdx * Arf.xScale + Arf.xDelta, .b = 540 + e.cdy * Arf.yScale };
			if( lifeMs > 370 )
				goto EANIM;

			if( lifeMs >= 0 )
				ePos = mPos;
			else if( !e.radius )
				if( lifeMs > -638 )		ePos = mPos, R = (lifeMs + 637) / 151.0;
				else					continue;
			else if( double x8d;  R += lifeMs * eSpeed / fmax(e.radius * 0.25, 6),  R < 0 )
				goto MISC_UPDATE;
			else
				x8d = (1-R) * (e.radius << 1),   /* 1/4 -> 1/8 */
				ePos.a = 360 * (e.initLoop / 64.0 + e.deltaLoop / 8.0 * R),  ePos = CosSin(ePos),
				ePos.a = mPos.a + x8d * ePos.a * Arf.xScale,
				ePos.b = mPos.b + x8d * ePos.b * Arf.yScale,
				R /= 0.237;
			R = fmin( R,1 );

			/* Update Echo */ {
				const GO echoGo   = ( lua_rawgeti(L, EGO, ++info.eUsed), dmScript::CheckGOInstance(L,-1) );
				const v4 echoTint = ( lua_rawgeti(L, ETINT, info.eUsed), dmScript::CheckVector4(L,-1)    );
				lua_pop(L, 2);

				if( lifeMs < -510 ) {
					echoTint -> setX(0.3).setY(0.3).setZ(0.3).setW(R);
					goto E_NSET_TSF;
				}
				if( lifeMs < -370 ) {
					const float C = 0.0005 * (lifeMs + 370) + 0.37;
					echoTint -> setX(C).setY(C).setZ(C).setW(R);
					goto E_NSET_TSF;
				}
				switch( e.status ) {
		[[likely]]	case NJUDGED:		case SPECIAL:
						echoTint -> setX(0.37).setY(0.37).setZ(0.37).setW(R);
						goto E_NSET_TSF;
					case NJUDGED_LIT:	case SPECIAL_LIT:
						echoTint -> setX(0.673).setY(0.673).setZ(0.673).setW(R);
						goto E_NSET_TSF;
					case HIT_LIT:
						echoTint -> setXYZ(HintHit).setW(R);
	   E_NSET_TSF:		SetPosition( echoGo, P3(ePos.a, ePos.b, 0.02) );
						SetScale( echoGo, 1.074 - 0.437 * R * (2-R) );
						break;
					case LOST:
						R = 0.573 - lifeMs * 0.00037,	echoTint -> setX(R).setW(1);
						R*= 0.51,						echoTint -> setY(R).setZ(R);
						goto E_LSET_TSF;
	  [[unlikely]]  case SPECIAL_LOST:
						R = 0.573 - lifeMs * 0.00037,	echoTint -> setX(R).setY(R).setZ(R).setW(1);
	   E_LSET_TSF:		SetPosition( echoGo, P3(ePos.a, ePos.b, 0.02) );
						SetScale( echoGo, 0.637 );
						break;
					default:   // Case HIT, no echoGo used
						--info.eUsed;
				}
			}   // This scope is required by goto

			/**/ MISC_UPDATE:
			if( e.status < HIT ) {
				if( lifeMs > -511 ) {
					const GO helper = ( lua_rawgeti(L, EH, ++info.xUsed), dmScript::CheckGOInstance(L,-1) );
					if( lifeMs < 0 )
						lua_pushnumber( L, R = 1 + lifeMs / 510.0 ), lua_rawseti(L, EHTINT, info.xUsed),
						SetScale( helper, 1.237 - R * (2-R) );
					else
						lua_pushnumber(L, 1), lua_rawseti(L, EHTINT, info.xUsed),
						SetScale( helper, 0.237 );
					SetPosition( helper, P3(mPos.a, mPos.b, 0.0625) );
					lua_pop(L, 1);
				}
			}
			else EANIM: if( e.status < LOST )
				info = renderAnim(L, info, mPos, lifeMs - e.deltaMs);
		}
	}
#endif

	return lua_pushinteger(L, info.wUsed), lua_pushinteger(L, info.hUsed), lua_pushinteger(L, info.eUsed),
		   lua_pushinteger(L, info.xUsed), lua_pushinteger(L, info.aUsed), lua_pushboolean(L, info.playH),
		   lua_pushboolean(L, info.playE), 7;
}