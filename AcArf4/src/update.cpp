//  Arf4 Update  //
#include <Arf4.h>
#include <span>

static const dmVMath::Vector3   // Hit, Early, Late
	Atint[]  {{1, 1, 1}, {0.3125, 0.5625, 0.63671875}, {0.63671875, 0.38671875, 0.3125}},
	Htint[]  {{0.837, 0.837, 0.837}, {0.37675, 0.67815, 0.767628125}, {0.767628125, 0.466228125, 0.37675}};
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
	const auto cosSin = CosSin({ degree * 0.5f });
	return Qt(0, 0, cosSin.b, cosSin.a);
}

static AuInfo renderWish(lua_State* L, AuInfo info, Duo Pos, const Duo zw) noexcept {
	if( Pos.b = 540 + Pos.b * Arf.yScale,  Pos.b >= -36  &&  Pos.b <= 1116 )
		if( Pos.a = 900 + Pos.a * Arf.xScale + Arf.xDelta,  Pos.a >= -36  &&  Pos.a <= 1836 ) {
			const auto wGo = ( lua_rawgeti(L, WGO, ++info.wUsed), dmScript::CheckGOInstance(L,-1) );
			if( lua_pop(L,1), info.sType )
				SetPosition( wGo, P3(Pos.a, Pos.b,.009f) ),  lua_pushnumber(L,-zw.b),  info.sType = 0;
			else
				SetPosition( wGo, P3(Pos.a, Pos.b, zw.a) ),  lua_pushnumber(L, zw.b);
			SetScale( wGo, 1.074 - 0.437 * zw.b * (2-zw.b) );   // Scale: 1.074 -> 0.637
			lua_rawseti(L, WTINT, info.wUsed);   // Tint
		}
	return info;
}

static AuInfo renderAnim(lua_State* L, AuInfo info, const Duo Pos, const uint16_t msPast) noexcept {
	if( msPast > 370 )	return info;
	const auto tint = ( lua_rawgeti(L, ATINT, ++info.aUsed), dmScript::CheckVector4(L, -1) );
	const auto lAgo = ( lua_rawgeti(L, AL, info.aUsed), dmScript::CheckGOInstance(L, -1) ),
			   rAgo = ( lua_rawgeti(L, AR, info.aUsed), dmScript::CheckGOInstance(L, -1) );
	lua_pop(L, 3);

	// Tint
	tint -> setXYZ( Atint[info.sType] );
	if( double w;  msPast < 73 )
		w = (msPast + 27) * 0.01,		tint -> setW( w * (2-w) );
	else
		w = (370-msPast) / 297.0,		tint -> setW( w * w );

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
	 * local wgo_used, hgo_used, ego_used, ehgo_used, ago_used, h_plhs, e_plhs = Arf4.UpdateArf(ms,
	 *       delay, wgos, hgos, egos, ehgos, agols, agors, wtints, htints, etints, ehtints, atints)
	 */
	AuInfo info;
	if( int32_t lastMs = Arf.msTime;  Arf.msTime = lua_tointeger(L, 1),  Arf.msTime >= Arf.before )
		return 0;
	else if( lastMs = Arf.msTime - lastMs,  lastMs > 0 )
		info.frameDt = lastMs > 33 ? 34 : lastMs;
	const double eSpeed = (PlayerSpeed * Arf.cSpeed + 11) / 375;	  double zDt[2] = {Arf.msTime * 1024.0};
	const double dSpeed = eSpeed / 1024 /* 1024x -> 4x */;			uint32_t zTimer = (Arf.msTime >> 2);

	/* Delta
	 * zDt = Scale * 1024, Dt = Scale * xSpeed
	 */
	if( const Delta lastDt = Arf.deltas.back();  zTimer >= lastDt.t )
		zDt[1] = lastDt.base + (Arf.msTime - lastDt.t * 4.0) * lastDt.absV;
	else {
		auto it = Arf.deltas.begin() + Arf.deltas[0].val;
			while( zTimer <  it[0].t )  { --it; }
			while( zTimer >= it[1].t )  { ++it; }
		if( const Delta thiz = *it;  thiz.base <= it[1].base )
			zDt[1] = thiz.base + (Arf.msTime - thiz.t * 4.0) * thiz.absV;
		else
			zDt[1] = thiz.base - (Arf.msTime - thiz.t * 4.0) * thiz.absV;
		Arf.deltas[0].val = it - Arf.deltas.begin();
	}
	const Index I = Arf.idx[ zTimer >> 8 ];

	/* Wish */
	for(uint32_t z = (zTimer >>= 9) == (Arf.before >> 11)  ?  Arf.wishes.size() : Arf.idx[zTimer+1].wSince,
				 i = Arf.idx[zTimer].wSince;  i < z;  ++i) {
		Wish w = Arf.wishes[i];

		/* Nodes */
		Point thiz, next;
		const auto nodes = &Arf.nodes[w.nSince];
		if( thiz = nodes[0],  Arf.msTime < thiz.ms )										break;

		if( w.nType )   // More than 2 Nodes
			if( thiz = nodes[w.nIndex],  Arf.msTime < thiz.ms ) {
				do	 --w.nIndex;
				while( thiz = nodes[w.nIndex],  Arf.msTime < thiz.ms );
				next = nodes[w.nIndex+1];
			}
			else {
				if( next = nodes[w.nIndex+1],  next.val  &&  Arf.msTime >= next.ms )
					do	 ++w.nIndex;
					while( next = nodes[w.nIndex+1],  next.val  &&  Arf.msTime >= next.ms );
				/**/if   ( thiz = nodes[w.nIndex],	 !next.val )							continue;
			}
		else if( next = nodes[1],  Arf.msTime >= next.ms )									continue;
		info.sType = w.isSpecial;

		Duo nodePos, frac { .a = Eased( (double)(Arf.msTime - thiz.ms) / (next.ms - thiz.ms), thiz.ease ),
							.b = (float)fmin(1, (Arf.msTime - nodes[0].ms) / 151.0) };
		if( float radius;  thiz.radius || next.radius )
			radius = thiz.radius + (next.radius - thiz.radius) * frac.a,
			nodePos = CosSin({ thiz.deg + (next.deg - thiz.deg) * frac.a }),
			nodePos.a = thiz.cdx + (next.cdx - thiz.cdx) * frac.a + radius * nodePos.a * 4,  // x4 → x16
			nodePos.b = thiz.cdy + (next.cdy - thiz.cdy) * frac.a + radius * nodePos.b * 2;  // x4 → x8
		else [[likely]]
			nodePos.a = thiz.cdx + (next.cdx - thiz.cdx) * frac.a,
			nodePos.b = thiz.cdy + (next.cdy - thiz.cdy) * frac.a;
		info = renderWish(L, info, nodePos, (frac.a = 0.01f, frac));

		/* WishChild */
		if( double wZdt, cQuo;  w.cType )
			if( const auto wChilds = &Arf.wishChilds[w.cSince];
				wZdt = zDt[w.withDt],  (wChilds[0].zDt - wZdt) * dSpeed < 32 /* 8*4 */ ) {

				// Manage cIndex
				while( w.cIndex  &&  wZdt < wChilds[w.cIndex-1].zDt )
					--w.cIndex;
				while( wChilds[w.cIndex].val  &&  wZdt >= wChilds[w.cIndex].zDt )
					++w.cIndex;
				Child c;

				// Traverse Subspan
				for( uint32_t ci = w.cIndex;  c = wChilds[ci],  c.val;  ci++ )
					if( (cQuo = 1 + (wZdt-c.zDt) * dSpeed / (c.radius>24 ? c.radius : 24 /* 6*4 */ )) <= 0 )
						break;
					else if( Duo cPos, cZw = { .a = 0.03f, .b = (float)(cQuo / 0.237) };  true )
						cQuo = w.isSpecial && !w.withDt  ?  Eased(cQuo, INSINE) : cQuo,
						cPos = CosSin({ 360 * (float)(c.initLoop / 64.0 + c.deltaLoop / 8.0 * cQuo) }),
						cQuo = (1-cQuo) * (c.radius << 1),   /* 1/4 -> 1/8 */
						cPos.a = nodePos.a + cQuo * cPos.a * 2,
						cPos.b = nodePos.b + cQuo * cPos.b,
						cZw.b = (frac.b < cZw.b) ? frac.b : (cZw.b < 1 ? cZw.b : 1),
						info = renderWish(L, info, cPos, cZw);
			}
		Arf.wishes[i] = w;   // `w` is a temporary value
	}

	/* Hint & Echo */
#ifndef AR_BUILD_VIEWER
	if( Arf.isAuto ) {   // There are much more boilerplate lines...
#endif
		for(const Body h : std::span(Arf.hints).subspan(I.hSince)) {
			const int16_t lifeMs = Arf.msTime - h.ms;
			if( lifeMs > +370 )		continue;   // +470 if not Auto
			if( lifeMs < -510 )		break;

			const Duo hPos  = { .a = 900 + h.cdx * Arf.xScale + Arf.xDelta,
								.b = 540 + h.cdy * Arf.yScale };
			const GO  hGo   = ( lua_rawgeti(L, HGO, ++info.hUsed), dmScript::CheckGOInstance(L,-1) );
			const v4  hTint = ( lua_rawgeti(L, HTINT, info.hUsed), dmScript::CheckVector4(L,-1)    );
			lua_pop(L, 2);

			if( float V;  lifeMs < -370 )
				V = lifeMs * 0.0001 - 0.037,			SetPosition( hGo, P3(hPos.a, hPos.b, V) ),
				V = 0.23 + (lifeMs + 510) * 0.001,		hTint -> setX(V).setY(V).setZ(V);
			else if( lifeMs < 0 )
				SetPosition( hGo, P3(hPos.a, hPos.b, -0.0573) ),
				hTint -> setX(0.37).setY(0.37).setZ(0.37);
			else
				(lifeMs < 101) ? SetPosition( hGo, P3(hPos.a, hPos.b, -0.0073) ), hTint -> setXYZ( *Htint ),
							 0 : --info.hUsed,   // Hint Go acquired, but not used
				info = renderAnim(L, info, hPos, lifeMs);
		}
		for(const Body e : std::span(Arf.echoes).subspan(I.eSince)) {
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
			else if( double x8d;  R += lifeMs * eSpeed / (e.radius>24 ? e.radius:24),  R < 0 )
				goto EHELP_AUTO;
			else
				x8d = (1-R) * (e.radius << 1),   /* 1/4 -> 1/8 */
				ePos.a = 360 * (e.initLoop / 64.0 + e.deltaLoop / 8.0 * R),  ePos = CosSin(ePos),
				ePos.a = mPos.a + x8d * ePos.a * Arf.xScale * 2,
				ePos.b = mPos.b + x8d * ePos.b * Arf.yScale,
				R /= 0.237;
			R = fmin( R,1 );

			// Update Echo
			if( lifeMs < 101 ) {
				const GO echoGo   = ( lua_rawgeti(L, EGO, ++info.eUsed), dmScript::CheckGOInstance(L,-1) );
				const v4 echoTint = ( lua_rawgeti(L, ETINT, info.eUsed), dmScript::CheckVector4(L,-1)    );
				lua_pop(L, 2);

				if( float C;  lifeMs < 0 )
					C = 0.001 * lifeMs + 0.74,
					C = C < 0.23f ? 0.23f  :  C > 0.37f ? 0.37f  :  C,
					echoTint -> setX(C).setY(C).setZ(C).setW(R),
					SetScale( echoGo, 0.937 - 0.37 * R*(2-R) );
				else
					echoTint -> setXYZ( *Htint ).setW(R),
					SetScale( echoGo, 0.567 );
				SetPosition( echoGo, P3(ePos.a, ePos.b, 0.02) );
			}

			// Update Anim & Helper
			if( lifeMs > 0 )
				info = renderAnim(L, info, mPos, lifeMs);
			else EHELP_AUTO: if( GO helper;  lifeMs > -511 )
				helper = ( lua_rawgeti(L, EH, ++info.xUsed), dmScript::CheckGOInstance(L,-1) ),
				lua_pushnumber( L, R = 1 + lifeMs / 510.0 ), lua_rawseti(L, EHTINT, info.xUsed),
				SetPosition( helper, P3(mPos.a, mPos.b, 0.0625) ),
				SetScale( helper, 1.237 - R * (2-R) ),
				lua_pop(L, 1);
		}
#ifndef AR_BUILD_VIEWER
	} else {
		for(const Body h : JudgeArfSweep(), std::span(Arf.hints).subspan(I.hSince)) {
			const int16_t lifeMs = Arf.msTime - h.ms;
			if( lifeMs > +470 )		continue;
			if( lifeMs < -510 )		break;

			const Duo hPos  = { .a = 900 + h.cdx * Arf.xScale + Arf.xDelta,
								.b = 540 + h.cdy * Arf.yScale };
			const GO  hGo   = ( lua_rawgeti(L, HGO,   ++info.hUsed), dmScript::CheckGOInstance(L,-1) );
			const v4  hTint = ( lua_rawgeti(L, HTINT, info.hUsed--), dmScript::CheckVector4(L,-1)    );
			lua_pop(L, 2);

			if( float V;  lifeMs < -370 )
				V = lifeMs * 0.0001 - 0.037,			SetPosition( hGo, P3(hPos.a, hPos.b, V) ),
				V = 0.23 + (lifeMs + 510) * 0.001,		hTint -> setX(V).setY(V).setZ(V),  ++info.hUsed;
			else if( lifeMs < 370 )
				if( (h.status == SPECIAL) && h.deltaMs ) [[unlikely]]   // LOST
					V = 0.573 - lifeMs * 0.00037,	hTint -> setX(V),
					V*= 0.51,						hTint -> setY(V).setZ(V),
					SetPosition( hGo, P3(hPos.a, hPos.b, -lifeMs * 0.00011) ),  ++info.hUsed;
				else switch( h.status ) {
					case NJUDGED:		case SPECIAL:
						hTint -> setX(0.37).setY(0.37).setZ(0.37),
						SetPosition( hGo, P3(hPos.a, hPos.b, -0.0637) ),		++info.hUsed;	continue;
					case NJUDGED_LIT:	case SPECIAL_LIT:
						hTint -> setX(0.573).setY(0.573).setZ(0.573);
						SetPosition( hGo, P3(hPos.a, hPos.b, -0.0573) ),		++info.hUsed;	continue;
					case HLIT:
						hTint -> setXYZ( *Htint );
						SetPosition( hGo, P3(hPos.a, hPos.b, -0.0073) ),		++info.hUsed;
					case HIT:			CASE_HIT:
						info = renderAnim(L, info, hPos, lifeMs - h.deltaMs);					continue;
	  [[unlikely]]  case HLIT_EC:
						hTint -> setXYZ( Htint[ 1 + (h.deltaMs > 0) ] );
						SetPosition( hGo, P3(hPos.a, hPos.b, -0.0037) ),		++info.hUsed;
	  [[unlikely]]  default:			CASE_HIT_EC:
						info.sType = 1 + (h.deltaMs > 0),
						info = renderAnim(L, info, hPos, lifeMs - h.deltaMs),
						info.sType = 0;
				}
			else switch( h.status ) {
				case HIT:		case HLIT:				goto CASE_HIT;
				case HIT_EC:	case HLIT_EC:			goto CASE_HIT_EC;
				default:;
			}
		}
		for(const Body e : std::span(Arf.echoes).subspan(I.eSince)) {
			const int16_t lifeMs = Arf.msTime - e.ms;
			if( lifeMs > 470 )
				continue;

			double R = 1;
			Duo ePos, mPos = { .a = 900 + e.cdx * Arf.xScale + Arf.xDelta, .b = 540 + e.cdy * Arf.yScale };
			if( lifeMs > 370 )
				if( e.status > SPECIAL_LIT )		goto EANIM;
				else								continue;

			if( lifeMs >= 0 )
				ePos = mPos;
			else if( !e.radius )
				if( lifeMs > -638 )					ePos = mPos, R = (lifeMs + 637) / 151.0;
				else								continue;
			else if( double x8d;  R += lifeMs * eSpeed / (e.radius>24 ? e.radius:24),  R < 0 )
				goto EHELP;
			else
				x8d = (1-R) * (e.radius << 1),   /* 1/4 -> 1/8 */
				ePos.a = 360 * (e.initLoop / 64.0 + e.deltaLoop / 8.0 * R),  ePos = CosSin(ePos),
				ePos.a = mPos.a + x8d * ePos.a * Arf.xScale * 2,
				ePos.b = mPos.b + x8d * ePos.b * Arf.yScale,
				R /= 0.237;
			R = fmin( R,1 );

			/* Update Echo */ {
				const GO echoGo   = ( lua_rawgeti(L, EGO, ++info.eUsed), dmScript::CheckGOInstance(L,-1) );
				const v4 echoTint = ( lua_rawgeti(L, ETINT, info.eUsed), dmScript::CheckVector4(L,-1)    );
				lua_pop(L, 2);

				if( float C;  lifeMs < -370 ) {
					C = lifeMs > -510 ? (0.001 * lifeMs + 0.74) : 0.23f;
					echoTint -> setX(C).setY(C).setZ(C).setW(R);
					goto EGOTSF;
				}
				if( lifeMs > 100  &&  e.status < NJUDGED_LIT )  [[unlikely]]  {
					R = 0.573 - lifeMs * 0.00037,	echoTint -> setX(R).setW(1),   // NJ: Lost
					R*= e.status ? 0.51 : 1,		echoTint -> setY(R).setZ(R),   // SP: Special Lost
					SetPosition( echoGo, P3(ePos.a, ePos.b, 0.02) ),
					SetScale( echoGo, 0.567 );
					continue;
				}
				switch( e.status ) {
					default:   // Case HIT, no echoGo used
						--info.eUsed;
						goto EANIM;
		[[likely]]	case NJUDGED:		case SPECIAL:
						echoTint -> setX(0.37).setY(0.37).setZ(0.37).setW(R);
						goto EGOTSF;
					case NJUDGED_LIT:	case SPECIAL_LIT:
						echoTint -> setX(0.673).setY(0.673).setZ(0.673).setW(R);
						goto EGOTSF;
					case HLIT:			case HLIT_EC:
						echoTint -> setXYZ( *Htint ).setW(R);
		   EGOTSF:		SetPosition( echoGo, P3(ePos.a, ePos.b, 0.02) );
						SetScale( echoGo, 0.937 - 0.37 * R * (2-R) );
				}
			}   // This scope is required by goto

			if( e.status < HIT )  /**/ EHELP:
				if( lifeMs > -511 ) {
					const GO helper = ( lua_rawgeti(L, EH, ++info.xUsed), dmScript::CheckGOInstance(L,-1) );
					if( lua_pop(L, 1),  lifeMs < 0 )
						lua_pushnumber( L, R = 1 + lifeMs / 510.0 ),  SetScale( helper, 1.237 - R * (2-R) );
					else
						lua_pushnumber(L, 1),						  SetScale( helper, 0.237 );
					SetPosition( helper, P3(mPos.a, mPos.b, 0.0625) );
					lua_rawseti(L, EHTINT, info.xUsed);
				} else {}
			else EANIM: info = renderAnim(L, info, mPos, lifeMs - e.deltaMs);
		}
	}
#endif
	if( uint32_t ft = lua_tointeger(L,2), lfms;  lua_toboolean(L,2)  &&  (ft += Arf.msTime) < Arf.before ) {
		const Index I2 = Arf.idx[ ft >> 10 ];
		for(const Body h : std::span(Arf.hints).subspan(I2.hSince))
			if( lfms = ft - h.ms,  info.playH |= (lfms < info.frameDt),  lfms >> 31 )
				break;
		for(const Body e : std::span(Arf.echoes).subspan(I2.eSince))
			if( lfms = ft - e.ms,  info.playE |= (lfms < info.frameDt) & e.status,  lfms >> 31 )
				break;
	}   // Auto HitSound, Delay [0,1000]

	return lua_pushinteger(L, info.wUsed), lua_pushinteger(L, info.hUsed), lua_pushinteger(L, info.eUsed),
		   lua_pushinteger(L, info.xUsed), lua_pushinteger(L, info.aUsed), lua_pushboolean(L, info.playH),
		   lua_pushboolean(L, info.playE), 7;
}