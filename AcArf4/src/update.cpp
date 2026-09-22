//  Arf4 Update  //
#include <Arf4.h>
static dmGameObject::HInstance *wGos, *hGos, *eGos, *xGos, *aLos, *aRos, *EOG;

static constexpr dmhash_t   // Sprite, tint, tint.w
	HCS = 0x95BB44E5E831FF13,  TINT = 0xCD91910279ABE2F6,  TINTW = 0xB554E29C664136CF;
static const dmVMath::Vector3   // Hit, Early, Late
	Atint[]  {{1, 1, 1}, {0.3125, 0.5625, 0.63671875}, {0.63671875, 0.38671875, 0.3125}},
	Htint[]  {{0.837, 0.837, 0.837}, {0.37675, 0.67815, 0.767628125}, {0.767628125, 0.466228125, 0.37675}};
struct Auinfo {
	uint64_t delta:6 = 0, wUsed:10 = 0, aUsed:10 = 0, eUsed:9 = 0, xUsed:9 = 0, hUsed:9 = 0;
	uint64_t sType:2 = false, playH:1 = false, playE:1 = false;
};

/* Utils & Render Methods */
static Auinfo renderWish(lua_State* L, Auinfo info, Ar::Duo Pos, const Ar::Duo zw) noexcept {
	if( dmGameObject::HInstance wGo;  Pos.b = 540 + Pos.b * Arf.yScale,  Pos.b >= -36  &&  Pos.b <= 1116 )
		if( Pos.a = 900 + Pos.a * Arf.xScale + Arf.xDelta,  Pos.a >= -36  &&  Pos.a <= 1836 )
			wGo = wGos[ info.wUsed ],
				SetPosition(wGo, { Pos.a, Pos.b, zw.a }),
				SetPropertyFromFloat(wGo, HCS, TINTW, zw.b),
				SetScale(wGo, ( 1.074f - 0.437f * (2-zw.b) * zw.b )),			// Scale: 1.074 -> 0.637
			lua_pushboolean(L, info.sType),  lua_rawseti(L, 3, ++info.wUsed);   // IsSpecial, UpdateArf #3
	return info;
}

static Auinfo renderAnim(Auinfo info, Ar::Duo PosQt, const uint16_t msPast, float Quo) noexcept {
	if( msPast < 371 ) {													   // Quo Premulted by 0.01f
		( msPast < 73 )  ?  (  Quo *= msPast + 27,			   Quo *= 2-Quo  ):
							(  Quo *= (370 - msPast) / 2.97f,  Quo *= Quo	 );
		const auto tint = dmVMath::Vector4( Atint[info.sType], Quo );

		// Tint & Position
		const auto lAgo = aLos[ info.aUsed   ];
			SetPropertyFromVector4(lAgo, HCS, TINT, tint);
			SetPosition(lAgo, { PosQt.a, PosQt.b, Quo = msPast * 0.0001f });
		const auto rAgo = aRos[ info.aUsed++ ];
			SetPropertyFromVector4(rAgo, HCS, TINT, tint);
			SetPosition(rAgo, { PosQt.a, PosQt.b, Quo - 0.00005f });

		// Rotation & Scale
		if( msPast < 193 )
			   Quo = msPast / 193.0f,						PosQt = Ar::CosSin({ 14 * Quo }),
			SetRotation(lAgo, { 0, 0, PosQt.b, PosQt.a }),
			   SetScale(lAgo, ( 1 + 0.637f * (2-Quo) * Quo ));
		else
			SetRotation(lAgo, { 0, 0, 0.24192189f, 0.97029572f }),
			   SetScale(lAgo, 1.637f);

		Quo = msPast / 370.0f,								PosQt = Ar::CosSin({ -4 * Quo });
			SetRotation(rAgo, { 0, 0, PosQt.b, PosQt.a });
			   SetScale(rAgo, ( 1 + 0.637f * (2-Quo) * Quo ));
	}	return info;
}

int Ar::Bind(lua_State* L) noexcept {
	/* Usage:
	 * Arf4.Bind(wgos, hgos, egos, ehgos, agols, agors)
	 */
	free( (dmGameObject::HInstance*)(wGos) );
		const uint16_t bindAgoRequired = Arf.hgoRequired + Arf.egoRequired,
					   sizeGosRequired = Arf.wgoRequired + Arf.hgoRequired * 3 + Arf.egoRequired * 4;
		wGos = (dmGameObject::HInstance*)malloc( sizeof(dmGameObject::HInstance) * sizeGosRequired );
		hGos = wGos + Arf.wgoRequired,	eGos = hGos + Arf.hgoRequired,	xGos = eGos + Arf.egoRequired,
		aLos = xGos + Arf.egoRequired,	aRos = aLos + bindAgoRequired,	 EOG = aLos + bindAgoRequired;
	auto it = wGos, xt = xGos;
		for( uint16_t i = 1;  it < hGos;  ++i, ++it )
			lua_rawgeti(L,1,i),		*it = dmScript::CheckGOInstance(L,7),			lua_settop(L,6);
		for( uint16_t i = 1;  it < eGos;  ++i, ++it )
			lua_rawgeti(L,2,i),		*it = dmScript::CheckGOInstance(L,7),			lua_settop(L,6);
		for( uint16_t i = 1;  it < xGos;  ++i, ++it, ++xt )
			lua_rawgeti(L,3,i),		*it = dmScript::CheckGOInstance(L,7),
			lua_rawgeti(L,4,i),		*xt = dmScript::CheckGOInstance(L,8),			lua_settop(L,6);
	it = aLos, xt = aRos;
		for( uint16_t i = 1;  it < aRos;  ++i, ++it, ++xt )
			lua_rawgeti(L,5,i),		*it = dmScript::CheckGOInstance(L,7),
			lua_rawgeti(L,6,i),		*xt = dmScript::CheckGOInstance(L,8),			lua_settop(L,6);
	return 0;
}

/* Main */
int Ar::UpdateArf(lua_State* L) noexcept {
	/* Usage:
	 * local wgou, hgou, egou, xgou, agou, hplhs, eplhs = Arf4.UpdateArf(ms, delay, wissp)
	 */
	Auinfo info;
	if( int32_t lastMs = Arf.msTime;  Arf.msTime = lua_tointeger(L,1),  Arf.msTime >= Arf.before )
		return 0;
	else if( lastMs = Arf.msTime - lastMs,  lastMs > 0 )
		info.delta = lastMs > 49 ? 50 : lastMs;

	const uint16_t wGrp = (Arf.msTime >> 11),  wFin = (Arf.before >> 11) == wGrp;
	float S[2] = { PlayerSpeed + (50.0f / 15000 * 4),  PlayerSpeed * Arf.cSpeed + (5.0f / 375) };
	Arf.vsTime ? 0: Arf.vsTime = Arf.msTime;

	/* Wish */
	for(Wish * wI = Arf.wish + Arf.idx[ wGrp ].wSince,
			 * wX = wFin ? (Wish*) Arf.child : Arf.wish + Arf.idx[ wGrp+1 ].wSince;  wI < wX;  ++wI) {
		Wish w = *wI;

		/* Nodes */
		Point thiz, next;
		const auto nodes = Arf.node + w.nSince;
		if( thiz = nodes[0],  Arf.msTime < thiz.ms )										break;

		if( w.nType )   // More than 2 Nodes
			if( thiz = nodes[w.nIndex],  Arf.msTime < thiz.ms ) {
				do	 --w.nIndex;
				while( thiz = nodes[w.nIndex],  Arf.msTime < thiz.ms );
				next = nodes[w.nIndex+1];
			} else {
				if( next = nodes[w.nIndex+1],  next.val  &&  Arf.msTime >= next.ms )
					do	 ++w.nIndex;
					while( next = nodes[w.nIndex+1],  next.val  &&  Arf.msTime >= next.ms );
				if( next.val == 0 )															continue;
			/**/	thiz = nodes[w.nIndex];
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
		info = renderWish(L, info, nodePos, ( frac.a = info.sType ? 0.00f : 0.01f,  frac ));
		info.sType = false;

		/* WishChild */
		if( float cQuo, wSf;  w.cType )
			if( Child *T, *C0 = Arf.child + w.cSince;  frac.am = w.withCs ? Arf.vsTime : Arf.msTime,
													   (C0->ms - frac.am) * (wSf = S[ w.withCs ]) < 32 ) {
				// Manage cIndex																 // 8*4
				T = C0 + w.cIndex;
					while(	T > C0  &&  frac.am < (T-1)->ms	)		--T;
					while(	T->val  &&  frac.am >= T->ms	)		++T;
				w.cIndex = T - C0;

				// Traverse Subspan
				for( Child C;  C = *T,  C.val;  ++T )									// 6*4
					if( cQuo = 1 + (frac.am - C.ms) * wSf / (C.radius > 24 ? C.radius : 24),  cQuo <= 0 )
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
		* wI = w;
	}	( Arf.vsTime > Arf.before ) ? ( Arf.vsTime = Arf.msTime ) : 0 ;   // TBD

	/* Hint & Echo */
#ifndef AR_BUILD_VIEWER
	if( Arf.isAuto ) {   // There are much more boilerplate lines...
#endif
		for( Body h, *hI = Arf.hint + Arf.idx[ Arf.msTime >> 10 ].hSince;  hI < Arf.echo;  ++hI ) {
			int16_t lfms = Arf.msTime - (h =* hI).ms;
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
		for( Body e, *eI = Arf.echo + Arf.idx[ Arf.vsTime >> 10 ].eSince;  eI < Arf.ceil;  ++eI ) {
			int16_t lfms = Arf.vsTime - (e =* eI).ms;
				if( lfms > 370 )
			/**/	continue;

			Duo ePos, mPos = { .a = 900 + e.cdx * Arf.xScale + Arf.xDelta,
							   .b = 540 + e.cdy * Arf.yScale };
			if( lfms >= 0 )
				ePos = mPos;
			else if( !e.radius )
				if( lfms > -638 )		ePos = mPos, *S = (lfms + 637) / 151.0f;
				else					continue;
			else if( float hDst;  *S = 1 + lfms * S[1] / (e.radius>24 ? e.radius:24),  *S < 0 )
				goto EHLP_AUTO;
			else
				hDst = (1-*S) * (e.radius << 2),   /* 1/4 -> 1/16 */
				ePos = CosSin({ e.initLoop * 5.625f + e.deltaLoop * 45 * (*S) }),
				ePos.a = mPos.a + hDst * ePos.a * Arf.xScale,
				ePos.b = mPos.b + hDst * ePos.b * Arf.yScale,
				*S /= 0.237f;
			*S = fminf( *S,1 );

			// Update Echo
			if( lfms < 101 ) {
				const auto echoGo = eGos[ info.eUsed++ ];

				if( float C;  lfms < 0 )
					C = 0.001f * lfms + 0.88f,
					C < 0.37f ? (C = 0.37f) : (C > 0.51f) ? (C = 0.51f) : 0,
					SetPropertyFromVector4(echoGo, HCS, TINT, { C, C, C, *S }),
					SetScale(echoGo, ( 1.074f - 0.437f * (2-*S) * (*S) ));
				else
					SetPropertyFromVector4(echoGo, HCS, TINT, { *Htint, *S }),
					SetScale(echoGo, 0.637f);
				SetPosition (echoGo, { ePos.a, ePos.b, 0.037f });
			}

			// Update Anim & Helper
			if( lfms > 0 )
				info = renderAnim(info, mPos, lfms, 0.0037f);
			else EHLP_AUTO: if( dmGameObject::HInstance xGo;  lfms > -511 )
				xGo = xGos[ info.xUsed++ ],
				SetPosition(xGo, { mPos.a, mPos.b, 0.037f }),
				SetPropertyFromFloat(xGo, HCS, TINTW, ( *S = 1 + lfms / 510.0f )),
				SetScale(xGo, ( 1.237f - (2-*S) * (*S) ));
		}
#ifndef AR_BUILD_VIEWER
	} else { /**/ JudgeArfSweep();
		for( Body h, *hI = Arf.hint + Arf.idx[ Arf.msTime >> 10 ].hSince;  hI < Arf.echo;  ++hI ) {
			int16_t lfms = Arf.msTime - (h =* hI).ms;
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
				if( float W;  h.status == SPECIAL == h.deltaMs )  [[unlikely]]  // LOST
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
		for( Body e, *eI = Arf.echo + Arf.idx[ Arf.vsTime >> 10 ].eSince;  eI < Arf.ceil;  ++eI ) {
			int16_t lfms = Arf.vsTime - (e =* eI).ms;
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
				if( lfms > -638 )					ePos = mPos, *S = (lfms + 637) / 151.0f;
				else								continue;
			else if( float hDst;  *S = 1 + lfms * S[1] / (e.radius>24 ? e.radius:24),  *S < 0 )
				goto EHLP;
			else
				hDst = (1-*S) * (e.radius << 2),   /* 1/4 -> 1/16 */
				ePos = CosSin({ e.initLoop * 5.625f + e.deltaLoop * 45 * (*S) }),
				ePos.a = mPos.a + hDst * ePos.a * Arf.xScale,
				ePos.b = mPos.b + hDst * ePos.b * Arf.yScale,
				*S /= 0.237f;
			*S = fminf( *S,1 );

			/* Update Echo */ {
				const auto echoGo = eGos[ info.eUsed++ ];

				if( float C;  lfms < -370 ) {
					C = lfms > -510 ? (0.001f * lfms + 0.88f) : 0.37f;
					SetPropertyFromVector4(echoGo, HCS, TINT, { C, C, C, *S });
					goto EGOTSF;
				}
				else if( lfms > 100  &&  e.status < NJUDGED_LIT )  [[unlikely]]  {
					*S = 0.573f - lfms * 0.00037f,			  // NJ: Lost
					 C = (*S) * (e.status ? 0.51f : 0.88f);   // SP: Special Lost
					SetPropertyFromVector4(echoGo, HCS, TINT, { *S, C, C, 1 });
					SetPosition(echoGo, { ePos.a, ePos.b, 0.037f });
					SetScale(echoGo, 0.637f);
					continue;
				}
				switch( e.status ) {
					default:   // Case HIT, no echoGo used
						info.eUsed--;
						goto EANIM;
		[[likely]]	case NJUDGED:		case SPECIAL:
						SetPropertyFromVector4(echoGo, HCS, TINT, { 0.51f, 0.51f, 0.51f, *S });
						goto EGOTSF;
					case NJUDGED_LIT:	case SPECIAL_LIT:
						SetPropertyFromVector4(echoGo, HCS, TINT, { 0.673f, 0.673f, 0.673f, *S });
						goto EGOTSF;
					case HLIT:			case HLIT_EC:
						SetPropertyFromVector4(echoGo, HCS, TINT, { *Htint, *S });
		   EGOTSF:		SetPosition(echoGo, { ePos.a, ePos.b, 0.037f });
						SetScale(echoGo, ( 1.074f - 0.437f * (2-*S) * (*S) ));
				}
			}   // This scope is required by goto

			if( e.status > SPECIAL_LIT )
				EANIM: info = renderAnim(info, mPos, lfms - e.deltaMs, 0.0037f);
			else EHLP: if( dmGameObject::HInstance xGo;  lfms > -511 )
				xGo = xGos[ info.xUsed++ ],
				( lfms < +0 ) ? ( *S = 1 + lfms / 510.0f,  SetScale(xGo, ( 1.237f + (*S - 2) * S[0] )),
														   SetPropertyFromFloat(xGo, HCS, TINTW, *S) ):
								(  SetScale(xGo, 0.237f),  SetPropertyFromFloat(xGo, HCS, TINTW, 1)  ),
				SetPosition(xGo, { mPos.a, mPos.b, 0.037f });
		}
	}
#endif

	// Auto HitSound, Delay [0,1000]
	if( uint32_t ft, lfms;  lua_toboolean(L,2)  &&  (ft = lua_tointeger(L,2) + Arf.msTime) < Arf.before ) {
		const Index I2 = Arf.idx[ ft >> 10 ];
		for(Body *hI = Arf.hint + I2.hSince;  hI < Arf.echo;  ++hI)
			if( lfms = ft - hI->ms,  info.playH |= (lfms < info.delta),  lfms >> 31 )
				break;
		for(Body *eI = Arf.echo + I2.eSince;  eI < Arf.ceil;  ++eI)
			if( lfms = ft - eI->ms,  info.playE |= (lfms < info.delta) & (eI -> status),  lfms >> 31 )
				break;
	}

	return lua_pushinteger(L, info.wUsed), lua_pushinteger(L, info.hUsed), lua_pushinteger(L, info.eUsed),
		   lua_pushinteger(L, info.xUsed), lua_pushinteger(L, info.aUsed), lua_pushboolean(L, info.playH),
		   lua_pushboolean(L, info.playE), 7;
}