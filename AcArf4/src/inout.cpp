//  Arf4 Inout  //
#include <Arf4.h>
#include <sys/stat.h>
#include <bitsery/traits/adapter_buffer.h>
#include <bitsery/traits/container_vector.h>
#include <bitsery/traits/compact_value.h>

/* Bitsery Settings */
#define Inout(TYPE, DETAILS)	template<typename S> void serialize(S& s, Arf4:: TYPE &its) {			   \
			  s.enableBitPacking( [&its](typename S::BPEnabledType& inout) { DETAILS ; } ); }
namespace bitsery {
	static constexpr auto CV = ext::CompactValueAsObject{};
	Inout( Wish,  inout.ext(its.val, CV); )		Inout( Body,  inout.ext(its.val, CV); )
	Inout( Point, inout.ext(its.val, CV); )		Inout( Index, inout.ext(its.val, CV); )
	Inout( Child, inout.ext(its.val, CV); )

	Inout( Fumen,
		inout.container(its.echoes, 131072);	inout.container(its.nodes, 262144);		// Consider "Equal"
		inout.container(its.hints, 32767);		inout.container(its.wishes, 16777216);	// Wishes
		inout.container(its.idx, 1024);			inout.container(its.wishChilds, 131072);
		inout.value8b(its.val);
	)

	struct A4CONF {
		static constexpr bool			 CheckAdapterErrors = false, CheckDataErrors = false;
		static constexpr EndiannessType  Endianness = DefaultConfig::Endianness;
	};
	using A4Encoder = Serializer< OutputBufferAdapter< std::vector<uint8_t>, A4CONF > >;
	using A4Decoder = Deserializer< InputBufferAdapter<const uint8_t*, A4CONF> >;
}

/* Inout Impls */
static void UndiffArf() {
	for( uint32_t cnt = Arf.hints.size(),  i = 1;  i < cnt;  ++i )
		Arf.hints[i].ms += Arf.hints[i-1].ms;
	for( uint32_t cnt = Arf.echoes.size(), i = 1;  i < cnt;  ++i )
		Arf.echoes[i].ms += Arf.echoes[i-1].ms,
		Arf.echoes[i].radius += Arf.echoes[i-1].radius,
		Arf.echoes[i].initLoop += Arf.echoes[i-1].initLoop,
		Arf.echoes[i].deltaLoop += Arf.echoes[i-1].deltaLoop;
	for( auto ls = Arf.wishChilds.begin(), tz = ls + 1;  tz < Arf.wishChilds.end();  ++ls, ++tz )
		(ls->val && tz->val) ? (tz->ms += ls->ms) : 0;
	for( auto ls = Arf.nodes.begin(), tz = ls + 1;  tz < Arf.nodes.end();  ++ls, ++tz )
		(ls->val && tz->val) ? (tz->ms += ls->ms) : 0;
}

int Ar::LoadArf(lua_State* L) {
	/* Usage:
	 * local before, objcnt, wgo_req, hgo_req, ego_req = Arf4.LoadArf(path, [is_auto])
	 */
	struct PseudoContext { dmResource::HFactory _, pF; };			// LUA_GLOBALSINDEX == -10002
	lua_pushnumber(L, 2744634527),  lua_rawget(L, -10002);			// Args -> hash"__script_context" | ctx
	const auto pCtx = (PseudoContext*)lua_touserdata(L, -1);		lua_pop(L, 1);
	const auto path = luaL_checkstring(L, 1);

	// Acquire Buffer
	uint8_t* pBuf;													// free() this.
	uint32_t bSize;													// [0] SEEK_SET  [2] SEEK_END
	if( dmResource::GetRaw(pCtx->pF, path, (void**)&pBuf, &bSize) != dmResource::RESULT_OK )
		if( FILE* pF = fopen(path, "rb");  pF )						// Open
			if( bSize = (fseek(pF, 0, 2), ftell(pF)),				// Size & Copying
				 pBuf = (fseek(pF, 0, 0), (uint8_t*)malloc(bSize)),  fread(pBuf, 1, bSize, pF) == bSize )
				(void)fclose(pF);
			else return fclose(pF), free(pBuf), 0;
		else return 0;

	// Decode & Return
	if( auto D = bitsery::A4Decoder(pBuf, bSize);  D.adapter().error() != bitsery::ReaderError::NoError )
		return free(pBuf), 0;
	else
		D.object( Arf = {} ), UndiffArf();   // Lazy clear only when the buffer is loaded successfully
	#ifndef AR_BUILD_VIEWER
		Arf.isAuto = lua_toboolean(L, 2),
		Arf.maxDt = (InputDelta > 63 ? 63 : InputDelta) + 37,
		Arf.minDt = Arf.maxDt - 74;
	#endif

	return lua_pushinteger(L, Arf.before),			lua_pushinteger(L, Arf.objectCount),
		   lua_pushinteger(L, Arf.wgoRequired),		lua_pushinteger(L, Arf.hgoRequired),
		   lua_pushinteger(L, Arf.egoRequired),		free(pBuf), 5;
}

int Ar::ExportArf(lua_State* L) {
	/* Usage:
	 * local str_or_nil = Arf4.ExportArf()
	 */
	for( auto& wish : Arf.wishes )
		wish.nIndex = 0, wish.cIndex = 0;

	/* Diff Fumen */
	for( int i = Arf.hints.size() - 1;	 i > 0;  --i )
		Arf.hints[i].ms -= Arf.hints[i-1].ms;
	for( int i = Arf.echoes.size() - 1;  i > 0;  --i )
		Arf.echoes[i].ms -= Arf.echoes[i-1].ms,
		Arf.echoes[i].radius -= Arf.echoes[i-1].radius,
		Arf.echoes[i].initLoop -= Arf.echoes[i-1].initLoop,
		Arf.echoes[i].deltaLoop -= Arf.echoes[i-1].deltaLoop;
	for( auto tz = Arf.wishChilds.end() - 1,  ls = tz - 1;  tz > Arf.wishChilds.begin();  --tz, --ls )
		(tz->val && ls->val) ? (tz->ms -= ls->ms) : 0;
	for( auto tz = Arf.nodes.end() - 1,  ls = tz - 1;  tz > Arf.nodes.begin();  --tz, --ls )
		(tz->val && ls->val) ? (tz->ms -= ls->ms) : 0;

	std::vector<uint8_t> buf;
	auto E = bitsery::A4Encoder(buf);
	E.object(Arf);

	#ifdef AR_BUILD_VIEWER
		UndiffArf();
	#endif

	size_t bufSize = ( E.adapter().flush(), E.adapter().writtenBytesCount() );
	return bufSize ? ( lua_pushlstring(L, (char*)&buf[0], bufSize), 1 ) : 0;
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