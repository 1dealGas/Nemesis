//  Arf4 Inout  //
#include <Arf4.h>
#include <sys/stat.h>

struct Awinfo {
	uint64_t  fmtNumber:12 = 0xA2F,
			  ceilDst:21, idxBys:13, hintBys:18, wishBys:23, childBys:20, nodeBys:21,
			  eGo:9, objectCount:16, echoBys:20, wGo:10, hGo:9;
};

static void* loadVzbuf(uint8_t* thiz, uint64_t num, uint64_t* out) noexcept {
	for( uint8_t *to = (thiz + num), ord = (num = 0);  thiz < to;  ++thiz )
		if( const uint8_t tnum = *thiz;  tnum & 0x80 )
			num |= ( tnum & 127 ) << ord,
			ord += 7;
		else
			*(out++) = num | (tnum << ord),
				 ord = num = 0;
	return out;
}

static uint8_t* writeVzbuf(void* thiz, const void* to, uint8_t* bufit) noexcept {
	for( uint64_t val;  thiz < to;  thiz = (uint64_t*)(thiz) + 1 )	{
		for( val = *(uint64_t*)(thiz);  val > 127;  val >>= 7 )
			*(bufit++) = (val & 127) | 0x80;
		*(bufit++) = (val);											}
	return bufit;
}

static void UndiffArf() noexcept {
	for( auto ls = Arf.node,  tz = (ls+1);  tz < (Ar::Point*)(Arf.wish);  ++ls, ++tz )
		(tz->val) ? (tz->ms += ls->ms) : 0;
	for( auto ls = Arf.child, tz = (ls+1);  tz < (Ar::Child*)(Arf.hint);  ++ls, ++tz )
		(tz->val) ? (tz->ms += ls->ms) : 0;
	for( auto ls = Arf.echo,  tz = (ls+1);  tz < Arf.ceil;  ++ls, ++tz )
		(tz) -> ms	   -= (ls) -> ms,		(tz) -> initLoop  -= (ls) -> initLoop,
		(tz) -> radius -= (ls) -> radius,	(tz) -> deltaLoop -= (ls) -> deltaLoop;
	for( auto ht = (Arf.hint) + 1;  ht < Arf.echo;  ++ht )
		(ht) -> ms  +=  (ht-1) -> ms;
}

int Ar::LoadArf(lua_State* L) noexcept {
	/* Usage:
	 * local before, objcnt, wgo_req, hgo_req, ego_req = Arf4.LoadArf(path, [is_auto])
	 */
	struct PseudoContext { dmResource::HFactory _, pF; };			// LUA_GLOBALSINDEX == -10002
	lua_pushnumber(L, 2744634527),  lua_rawget(L, -10002);			// Args -> hash"__script_context" | ctx
	const auto pCtx = (PseudoContext*)lua_touserdata(L, -1);		lua_pop(L, 1);
	const auto path = luaL_checkstring(L, 1);

	// Acquire Buffer
	uint32_t bSize;													// [0] SEEK_SET  [2] SEEK_END
	 uint8_t *pBuf, *cBuf;											// pBuf to be released via free()
	if( dmResource::GetRaw(pCtx->pF, path, (void**)&pBuf, &bSize) != dmResource::RESULT_OK )
		if( FILE* pF = fopen(path, "rb");  pF )						// Open
			if( bSize = (fseek(pF, 0, 2), ftell(pF)),				// Size & Copying
				 pBuf = (fseek(pF, 0, 0), (uint8_t*)malloc(bSize)),  fread(pBuf, 1, bSize, pF) == bSize )
				(void)fclose(pF);
			else return fclose(pF), free(pBuf), 0;
		else return 0;

	// Decode Varint-Zipped Buffer
	Awinfo H;
	if( bSize < 24 || ( H = *(Awinfo*)pBuf ).fmtNumber != 0xA2F
				   || bSize < 24 + H.idxBys + H.wishBys + H.nodeBys + H.childBys + H.hintBys + H.echoBys )
		return free(pBuf), 0;
	free( Arf.idx );

	Arf = {  .idx = (Index*)malloc( H.ceilDst << 3 ),
			.node = (Point*)loadVzbuf( cBuf  = pBuf + 24,  H.idxBys,   (uint64_t*)(Arf.idx)  ),
			.wish = (Wish*) loadVzbuf( cBuf += H.idxBys,   H.nodeBys,  (uint64_t*)(Arf.node) ),
		   .child = (Child*)loadVzbuf( cBuf += H.nodeBys,  H.wishBys,  (uint64_t*)(Arf.wish) ),
			.hint = (Body*) loadVzbuf( cBuf += H.wishBys,  H.childBys, (uint64_t*)(Arf.child)),
			.echo = (Body*) loadVzbuf( cBuf += H.childBys, H.hintBys,  (uint64_t*)(Arf.hint) ),
			.ceil = (Body*) loadVzbuf( cBuf += H.hintBys,  H.echoBys,  (uint64_t*)(Arf.echo) ),
			.wgoRequired = H.wGo,				.hgoRequired = H.hGo,
			.egoRequired = H.eGo,				.objectCount = H.objectCount					};
	UndiffArf();

	// Config & Return
	const uint32_t LH = (Arf.hint < Arf.echo ? Arf.echo[-1].ms + 470 : 0),
				   LE = (Arf.echo < Arf.ceil ? Arf.ceil[-1].ms + 470 : 0);
		   Arf.before = ( LH > LE ? LH : LE );

	for( auto nit = Arf.node;  nit < (Point*)Arf.wish;  ++nit )
		if( const auto ms = nit->ms;  Arf.before < ms )
			Arf.before = ms;

	#ifndef AR_BUILD_VIEWER
		Arf.isAuto = lua_toboolean(L, 2);
	#endif

	return lua_pushinteger(L, Arf.before),			lua_pushinteger(L, Arf.objectCount),
		   lua_pushinteger(L, Arf.wgoRequired),		lua_pushinteger(L, Arf.hgoRequired),
		   lua_pushinteger(L, Arf.egoRequired),		free(pBuf), 5;
}

int Ar::ExportArf(lua_State* L) noexcept {
	/* Usage:
	 * local str_or_nil = Arf4.ExportArf()
	 */
	for( auto wit = Arf.wish;  wit < (Wish*)Arf.child;  ++wit )
		wit->val &= 0x7F'FFFF'FFFF;
	const uint32_t ceilDst = Arf.ceil - (Body*)Arf.idx;

	// Diff Fumen
	for( auto tz = (Point*)(Arf.wish) - 1,  ls = (tz-1);  tz > Arf.node;   --tz, --ls )
		(tz->val) ? (tz->ms -= ls->ms) : 0;
	for( auto tz = (Child*)(Arf.hint) - 2,  ls = (tz-1);  tz > Arf.child;  --tz, --ls )   // Child End 0
		(tz->val) ? (tz->ms -= ls->ms) : 0;
	for( auto tz = (Arf.ceil) - 1,  ls = (tz-1);  tz > Arf.echo;  --tz, --ls )
		(tz) -> ms	   -= (ls) -> ms,		(tz) -> initLoop  -= (ls) -> initLoop,
		(tz) -> radius -= (ls) -> radius,	(tz) -> deltaLoop -= (ls) -> deltaLoop;
	for( auto ht = (Arf.echo) - 1;  ht > Arf.hint;  --ht )
		(ht) -> ms  -=  (ht-1) -> ms;

	// Write & Return
	uint8_t* oIs[8] = { (uint8_t*) malloc( (ceilDst << 3) + 24 ) };		oIs[1] = oIs[0] + 24;
			 oIs[2] = writeVzbuf( Arf.idx,	 Arf.node,  oIs[1] );
			 oIs[3] = writeVzbuf( Arf.node,  Arf.wish,  oIs[2] );
			 oIs[4] = writeVzbuf( Arf.wish,  Arf.child, oIs[3] );
			 oIs[5] = writeVzbuf( Arf.child, Arf.hint,  oIs[4] );
			 oIs[6] = writeVzbuf( Arf.hint,  Arf.echo,  oIs[5] );
			 oIs[7] = writeVzbuf( Arf.echo,  Arf.ceil,  oIs[6] );
	#ifdef AR_BUILD_VIEWER
		UndiffArf();
	#endif

	*(Awinfo*)(*oIs) = { .ceilDst = ceilDst,
						  .idxBys = (uint64_t)(oIs[2] - oIs[1]),	.hintBys = (uint64_t)(oIs[6] - oIs[5]),
						 .wishBys = (uint64_t)(oIs[4] - oIs[3]),   .childBys = (uint64_t)(oIs[5] - oIs[4]),
						 .nodeBys = (uint64_t)(oIs[3] - oIs[2]),		.eGo = Arf.egoRequired,
					 .objectCount = Arf.objectCount,				.echoBys = (uint64_t)(oIs[7] - oIs[6]),
							 .wGo = Arf.wgoRequired,					.hGo = Arf.hgoRequired			  };
	return lua_pushlstring( L, (char*)(*oIs), oIs[7] - (*oIs) ),
		   free( *oIs ),
		   1;
}

#ifdef AR_BUILD_VIEWER
int Ar::GetFileMtime(lua_State* L) noexcept {
	/* Usage:
	 * local modtime_or_nil = Arf4.GetFileMtime(path)
	 */
	struct stat FS;
	return stat( luaL_checkstring(L,1), &FS ) ? 0 : ( lua_pushinteger(L, FS.st_mtime), 1 );
}
#endif