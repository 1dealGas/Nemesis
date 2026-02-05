//  Arf4 Update  //
#include <Arf4.h>
#include <span>

static const dmVMath::Vector3   // Hit, Early, Late
	Atint[]  {{1, 1, 1}, {0.3125, 0.5625, 0.63671875}, {0.63671875, 0.38671875, 0.3125}},
	Htint[]  {{0.837, 0.837, 0.837}, {0.37675, 0.67815, 0.767628125}, {0.767628125, 0.466228125, 0.37675}};
static constexpr dmhash_t   // Sprite, tint, tint.w
	HCS = 0x95BB44E5E831FF13,  TINT = 0xCD91910279ABE2F6,  TINTW = 0xB554E29C664136CF;
static std::vector<dmGameObject::HInstance> wGos, hGos, eGos, xGos, aGosl, aGosr;

struct AuInfo {
	uint64_t frameDt:6 = 0, wUsed:10 = 0, aUsed:10 = 0, eUsed:9 = 0, xUsed:9 = 0, hUsed:9 = 0;
	uint64_t sType:2 = false, playH:1 = false, playE:1 = false;
};

/* Utils & Render Methods */
static AuInfo renderWish(lua_State* L, AuInfo info, Ar::Duo Pos, const Ar::Duo zw) noexcept {
	if( dmGameObject::HInstance wGo;  Pos.b = 540 + Pos.b * Arf.yScale,  Pos.b >= -36  &&  Pos.b <= 1116 )
		if( Pos.a = 900 + Pos.a * Arf.xScale + Arf.xDelta,  Pos.a >= -36  &&  Pos.a <= 1836 )
			wGo = wGos[ info.wUsed ],
				SetPropertyFromFloat(wGo, HCS, TINTW, zw.b),
				SetPosition(wGo, { Pos.a, Pos.b, (info.sType ? 0 : zw.a) }),
				SetScale(wGo, ( 1.074f - 0.437f * (2-zw.b) * zw.b )),			// Scale: 1.074 -> 0.637
			lua_pushboolean(L, info.sType),  lua_rawseti(L, 3, ++info.wUsed);   // IsSpecial, UpdateArf #3
	return info;
}

static AuInfo renderAnim(AuInfo info, Ar::Duo PosQt, const uint16_t msPast, float Quo) noexcept {
	if( msPast < 371 ) {
		( msPast < 73 )  ?  (  Quo *= msPast + 27,				Quo *= 2-Quo  ):
							(  Quo *= (370 - msPast) / 2.97f,	Quo *= Quo	  );
		const auto tint = dmVMath::Vector4( Atint[info.sType],  Quo );

		// Tint & Position
		const auto lAgo = aGosl[ info.aUsed   ];
			SetPropertyFromVector4(lAgo, HCS, TINT, tint);
			SetPosition(lAgo, { PosQt.a, PosQt.b, Quo = msPast * 0.0001f });
		const auto rAgo = aGosr[ info.aUsed++ ];
			SetPropertyFromVector4(rAgo, HCS, TINT, tint);
			SetPosition(rAgo, { PosQt.a, PosQt.b, Quo - 0.00005f });

		// Rotation & Scale
		if( msPast < 193 )
			   Quo = msPast / 193.0f,						PosQt = Ar::CosSin({ 22.5f + 14 * Quo }),
			SetRotation(lAgo, { 0, 0, PosQt.b, PosQt.a }),
			   SetScale(lAgo, ( 1 + 0.637f * (2-Quo) * Quo ));
		else
			SetRotation(lAgo, { 0, 0, 0.59482278f, 0.80385686f }),
			   SetScale(lAgo, 1.637f);

		Quo = msPast / 370.0,								PosQt = Ar::CosSin({ 22.5f - 8 * Quo });
			SetRotation(rAgo, { 0, 0, PosQt.b, PosQt.a });
			   SetScale(rAgo, ( 1 + 0.637f * (2-Quo) * Quo ));
	}	return info;
}

int Ar::Bind(lua_State* L) noexcept {
	/* Usage:
	 * Arf4.Bind(wgos, hgos, egos, ehgos, agols, agors)
	 */
	const uint16_t bindAgoRequired = Arf.hgoRequired + Arf.egoRequired;
	for(  uint16_t z = Arf.wgoRequired,  i = (wGos.resize(z), 1);  i <= z;  ++i  )
		lua_rawgeti(L,1,i),    wGos[i-1] = dmScript::CheckGOInstance(L,7),				lua_settop(L,6);
	for(  uint16_t z = Arf.hgoRequired,  i = (hGos.resize(z), 1);  i <= z;  ++i  )
		lua_rawgeti(L,2,i),    hGos[i-1] = dmScript::CheckGOInstance(L,7),				lua_settop(L,6);
	for(  uint16_t z = Arf.egoRequired,  i = (eGos.resize(z), xGos.resize(z), 1);	 i <= z;  ++i  )
		lua_rawgeti(L,3,i),    eGos[i-1] = dmScript::CheckGOInstance(L,7),
		lua_rawgeti(L,4,i),    xGos[i-1] = dmScript::CheckGOInstance(L,8),				lua_settop(L,6);
	for(  uint16_t z = bindAgoRequired,  i = (aGosl.resize(z), aGosr.resize(z), 1);	 i <= z;  ++i  )
		lua_rawgeti(L,5,i),   aGosl[i-1] = dmScript::CheckGOInstance(L,7),
		lua_rawgeti(L,6,i),   aGosr[i-1] = dmScript::CheckGOInstance(L,8),				lua_settop(L,6);
	return 0;
}

/* Main */
int Ar::UpdateArf(lua_State* L) noexcept {
	/* Usage:
	 * local wgou, hgou, egou, xgou, agou, hplhs, eplhs = Arf4.UpdateArf(ms, delay, wissp)
	 */
	AuInfo info;
	if( int32_t lastMs = Arf.msTime;  Arf.msTime = lua_tointeger(L, 1),  Arf.msTime >= Arf.before )
		return 0;
	else if( lastMs = Arf.msTime - lastMs,  lastMs > 0 )
		info.frameDt = lastMs > 49 ? 50 : lastMs;
	const Index I = Arf.idx[ Arf.msTime >> 10 ];

	/* Wish */
	const float SPD[2] = { PlayerSpeed + 0.01333333f,  PlayerSpeed * Arf.cSpeed + 0.01333333f };   // 5÷375
	for(uint32_t o = Arf.msTime >> 11,	 z = o ^ Arf.before >> 11 ? Arf.idx[o+1].wSince : Arf.wishes.size(),
				 i = Arf.idx[o].wSince;  i < z;  ++i) {
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

		Duo nodePos, frac { .a = Eased( (float)(Arf.msTime - thiz.ms) / (next.ms - thiz.ms), thiz.ease ),
							.b = fminf( 1, (Arf.msTime - nodes[0].ms) / 151.0f ) };
		if( float hr;  thiz.radius | next.radius )
			hr = ( thiz.radius + (next.radius - thiz.radius) * frac.a ) * 4,		  // x4 -> x16
			nodePos = CosSin({ thiz.deg + (next.deg - thiz.deg) * frac.a }),
			nodePos.a = thiz.cdx + (next.cdx - thiz.cdx) * frac.a + hr * nodePos.a,   // cos
			nodePos.b = thiz.cdy + (next.cdy - thiz.cdy) * frac.a + hr * nodePos.b;   // sin
		else [[likely]]
			nodePos.a = thiz.cdx + (next.cdx - thiz.cdx) * frac.a,
			nodePos.b = thiz.cdy + (next.cdy - thiz.cdy) * frac.a;

		info.sType = w.isSpecial;
		info = renderWish(L, info, nodePos, (frac.a = 0.01f, frac));
		info.sType = false;

		/* WishChild */
		if( float cQuo, wSf;  w.cType )															   // 8*4
			if( auto it =& Arf.wishChilds[w.cSince];  (it->ms - Arf.msTime) * (wSf = SPD[w.withCs]) < 32 ) {
				/**/ it += w.cIndex;

				// Manage cIndex
				while( w.cIndex  &&  Arf.msTime < (it-1)->ms )
					--it, --w.cIndex;
				while( it->val   &&  Arf.msTime >= it->ms )
					++it, ++w.cIndex;

				// Traverse Subspan
				for( Child C;  C = *it,  C.val;  ++it )									// 6*4
					if( cQuo = 1 + (Arf.msTime - C.ms) * wSf / (C.radius > 24 ? C.radius : 24),  cQuo <= 0 )
						break;
					else if( Duo cPos, cZw = { .a = 0.02f, .b = cQuo / 0.237f };  true )
						(w.isSpecial &! w.withCs) ? (cQuo = Eased(cQuo, INSINE)) : 0,
							cPos = CosSin({ C.initLoop * 5.625f + C.deltaLoop * 45 * cQuo }),
							cQuo = (1-cQuo) * (C.radius << 2),		// 1/4 -> 1/16
							cPos.a = nodePos.a + cQuo * cPos.a,
							cPos.b = nodePos.b + cQuo * cPos.b,
						(cZw.b > frac.b) ? (cZw.b = frac.b) : 0,	// frac.b <= 1
							info = renderWish(L, info, cPos, cZw);
			}
		Arf.wishes[i] = w;   // `w` is a temporary value
	}

	/* Hint & Echo */
#ifndef AR_BUILD_VIEWER
	if( Arf.isAuto ) {   // There are much more boilerplate lines...
#endif
		for( const Body h : std::span(Arf.hints).subspan(I.hSince) ) {
			int16_t lfms = Arf.msTime - h.ms;
				if( lfms > +370 )		continue;   // +470 if not Auto
				if( lfms < -510 )		break;
			const Duo hPos = { .a = 900 + h.cdx * Arf.xScale + Arf.xDelta,
							   .b = 540 + h.cdy * Arf.yScale };
			const auto hGo = hGos[ info.hUsed++ ];

			if( float V;  lfms < -370 )
				V = 0.23f + (lfms + 510) * 0.001f,	SetPropertyFromVector4(hGo, HCS, TINT, { V, V, V, 1 }),
				V = lfms * 0.0001f - 0.037f,		SetPosition(hGo, { hPos.a, hPos.b, V });
			else if( lfms < 0 )
				SetPropertyFromVector4(hGo, HCS, TINT, { 0.37f, 0.37f, 0.37f, 1 }),
				SetPosition(hGo, { hPos.a, hPos.b, -0.0573f });
			else
				( lfms < 101 ) ? SetPropertyFromVector4(hGo, HCS, TINT, { *Htint, 1 }),
								 SetPosition(hGo, { hPos.a, hPos.b, -0.0073f }),
							 0 : --info.hUsed,   // Hint Go acquired, but not used
				info = renderAnim(info, hPos, lfms, 0.0088f);
		}
		for( float R;  const Body e : std::span(Arf.echoes).subspan(I.eSince) ) {
			int16_t lfms = Arf.msTime - e.ms;
				if( lfms > 370 )
			/**/	continue;

			Duo ePos, mPos = { .a = 900 + e.cdx * Arf.xScale + Arf.xDelta,
							   .b = 540 + e.cdy * Arf.yScale };
			if( lfms >= 0 )
				ePos = mPos;
			else if( !e.radius )
				if( lfms > -638 )		ePos = mPos, R = (lfms + 637) / 151.0f;
				else					continue;
			else if( float hDst;  R = 1 + lfms * SPD[1] / (e.radius>24 ? e.radius:24),  R < 0 )
				goto EHLP_AUTO;
			else
				hDst = (1-R) * (e.radius << 2),   /* 1/4 -> 1/16 */
				ePos = CosSin({ e.initLoop * 5.625f + e.deltaLoop * 45 * R }),
				ePos.a = mPos.a + hDst * ePos.a * Arf.xScale,
				ePos.b = mPos.b + hDst * ePos.b * Arf.yScale,
				R /= 0.237f;
			R = fminf( R,1 );

			// Update Echo
			if( lfms < 101 ) {
				const auto echoGo = eGos[ info.eUsed++ ];

				if( float C;  lfms < 0 )
					C = 0.001f * lfms + 0.88f,
					C < 0.37f ? (C = 0.37f) : (C > 0.51f) ? (C = 0.51f) : 0,
					SetPropertyFromVector4(echoGo, HCS, TINT, { C, C, C, R }),
					SetScale(echoGo, ( 1.074f - 0.437f * (2-R) * R ));
				else
					SetPropertyFromVector4(echoGo, HCS, TINT, { *Htint,  R }),
					SetScale(echoGo, 0.637f);
				SetPosition (echoGo, { ePos.a, ePos.b, 0.037f });
			}

			// Update Anim & Helper
			if( lfms > 0 )
				info = renderAnim(info, mPos, lfms, 0.0037f);
			else EHLP_AUTO: if( dmGameObject::HInstance xGo;  lfms > -511 )
				xGo = xGos[ info.xUsed++ ],
				SetPosition(xGo, { mPos.a, mPos.b, 0.037f }),
				SetPropertyFromFloat(xGo, HCS, TINTW, ( R = 1 + lfms / 510.0f )),
				SetScale(xGo, ( 1.237f - (2-R) * R ));
		}
#ifndef AR_BUILD_VIEWER
	} else {
		for( const Body h : JudgeArfSweep(), std::span(Arf.hints).subspan(I.hSince) ) {
			int16_t lfms = Arf.msTime - h.ms;
				if( lfms > +470 )		continue;
				if( lfms < -510 )		break;
			const Duo hPos = { .a = 900 + h.cdx * Arf.xScale + Arf.xDelta,
							   .b = 540 + h.cdy * Arf.yScale };
			const auto hGo = hGos[ info.hUsed ];

			if( float V;  lfms < -370 )
				V = 0.23f + (lfms + 510) * 0.001f,
				SetPropertyFromVector4(hGo, HCS, TINT, { V, V, V, 1 }),
				SetPosition(hGo, { hPos.a, hPos.b, lfms * 0.0001f - 0.037f }),  ++info.hUsed;
			else if( lfms < 370 )
				if( float W;  (h.status == SPECIAL) && h.deltaMs )  [[unlikely]]  // LOST
					V = 0.573f - lfms * 0.00037f,	W = V * 0.51f,
					SetPropertyFromVector4(hGo, HCS, TINT, { V, W, W, 1 }),
					SetPosition(hGo, { hPos.a, hPos.b, -lfms * 0.00011f }),		++info.hUsed;
				else switch( h.status ) {
					case NJUDGED:		case SPECIAL:
						SetPropertyFromVector4(hGo, HCS, TINT, { 0.37f, 0.37f, 0.37f, 1 }),
						SetPosition(hGo, { hPos.a, hPos.b, -0.0637f }),			++info.hUsed;	continue;
					case NJUDGED_LIT:	case SPECIAL_LIT:
						SetPropertyFromVector4(hGo, HCS, TINT, { 0.573f, 0.573f, 0.573f, 1 }),
						SetPosition(hGo, { hPos.a, hPos.b, -0.0573f }),			++info.hUsed;	continue;
					case HLIT:
						SetPropertyFromVector4(hGo, HCS, TINT, { *Htint, 1 }),
						SetPosition(hGo, { hPos.a, hPos.b, -0.0073f }),			++info.hUsed;
					case HIT:			CASE_HIT:
						info = renderAnim(info, hPos, lfms - h.deltaMs, 0.0088f);				continue;
	  [[unlikely]]  case HLIT_EC:
						SetPropertyFromVector4(hGo, HCS, TINT, { Htint[ 1 + (h.deltaMs > 0) ],  1 }),
						SetPosition(hGo, { hPos.a, hPos.b, -0.0037f }),			++info.hUsed;
	  [[unlikely]]  default:			CASE_HIT_EC:
						info.sType = 1 + (h.deltaMs > 0),
						info = renderAnim(info, hPos, lfms - h.deltaMs, 0.0088f),
						info.sType = 0;
				}
			else switch( h.status ) {
				case HIT:		case HLIT:				goto CASE_HIT;
				case HIT_EC:	case HLIT_EC:			goto CASE_HIT_EC;
				default:;
			}
		}
		for( float R;  const Body e : std::span(Arf.echoes).subspan(I.eSince) ) {
			int16_t lfms = Arf.msTime - e.ms;
				if( lfms > 470 )
			/**/	continue;

			Duo ePos, mPos = { .a = 900 + e.cdx * Arf.xScale + Arf.xDelta,
							   .b = 540 + e.cdy * Arf.yScale };
			if( lfms > 370 )
				if( e.status > SPECIAL_LIT )		goto EANIM;
				else								continue;
			if( lfms >= 0 )
				ePos = mPos;
			else if( !e.radius )
				if( lfms > -638 )					ePos = mPos, R = (lfms + 637) / 151.0f;
				else								continue;
			else if( float hDst;  R = 1 + lfms * SPD[1] / (e.radius>24 ? e.radius:24),  R < 0 )
				goto EHLP;
			else
				hDst = (1-R) * (e.radius << 2),   /* 1/4 -> 1/16 */
				ePos = CosSin({ e.initLoop * 5.625f + e.deltaLoop * 45 * R }),
				ePos.a = mPos.a + hDst * ePos.a * Arf.xScale,
				ePos.b = mPos.b + hDst * ePos.b * Arf.yScale,
				R /= 0.237f;
			R = fminf( R,1 );

			/* Update Echo */ {
				const auto echoGo = eGos[ info.eUsed++ ];

				if( float C;  lfms < -370 ) {
					C = lfms > -510 ? (0.001f * lfms + 0.88f) : 0.37f;
					SetPropertyFromVector4(echoGo, HCS, TINT, { C, C, C, R });
					goto EGOTSF;
				}
				else if( lfms > 100  &&  e.status < NJUDGED_LIT )  [[unlikely]]  {
					R = 0.573f - lfms * 0.00037f,		  // NJ: Lost
					C = R * (e.status ? 0.51f : 0.88f);   // SP: Special Lost
					SetPropertyFromVector4(echoGo, HCS, TINT, { R, C, C, 1 });
					SetPosition(echoGo, { ePos.a, ePos.b, 0.037f });
					SetScale(echoGo, 0.637f);
					continue;
				}
				switch( e.status ) {
					default:   // Case HIT, no echoGo used
						--info.eUsed;
						goto EANIM;
		[[likely]]	case NJUDGED:		case SPECIAL:
						SetPropertyFromVector4(echoGo, HCS, TINT, { 0.51f, 0.51f, 0.51f, R });
						goto EGOTSF;
					case NJUDGED_LIT:	case SPECIAL_LIT:
						SetPropertyFromVector4(echoGo, HCS, TINT, { 0.673f, 0.673f, 0.673f, R });
						goto EGOTSF;
					case HLIT:			case HLIT_EC:
						SetPropertyFromVector4(echoGo, HCS, TINT, { *Htint, R });
		   EGOTSF:		SetPosition(echoGo, { ePos.a, ePos.b, 0.037f });
						SetScale(echoGo, ( 1.074f - 0.437f * (2-R) * R ));
				}
			}   // This scope is required by goto

			if( e.status > SPECIAL_LIT )
				EANIM: info = renderAnim(info, mPos, lfms - e.deltaMs, 0.0037f);
			else EHLP: if( dmGameObject::HInstance xGo;  lfms > -511 )
				xGo = xGos[ info.xUsed++ ],
				( lfms < +0 ) ? ( R = 1 + lfms / 510.0f,  SetScale(xGo, ( 1.237f - (2-R) * R )),
														  SetPropertyFromFloat(xGo, HCS, TINTW, R) ):
								( SetScale(xGo, 0.237f),  SetPropertyFromFloat(xGo, HCS, TINTW, 1) ),
				SetPosition(xGo, { mPos.a, mPos.b, 0.037f });
		}
	}
#endif
	if( uint32_t ft, lfms;  lua_toboolean(L,2)  &&  (ft = lua_tointeger(L,2) + Arf.msTime) < Arf.before ) {
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