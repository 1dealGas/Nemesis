//  Arf4 Inout  //
#include <Arf4.h>
#include <sys/stat.h>
#include <bitsery/traits/adapter_buffer.h>
#include <bitsery/traits/container_vector.h>
#include <bitsery/traits/compact_value.h>
#include <dmsdk/dlib/crypt.h>

/* Bitsery Settings */
#define Inout(TYPE, DETAILS)	template<typename S> void serialize(S& s, Arf4:: TYPE &its) {			   \
			  s.enableBitPacking( [&its](typename S::BPEnabledType& inout) { DETAILS ; } ); }
namespace bitsery {
	static constexpr auto CV = ext::CompactValueAsObject{};
	Inout( Index, inout.ext(its.val, CV); )		Inout( Point, inout.ext(its.val, CV); )
	Inout( Child, inout.ext(its.val, CV); )		Inout( Delta, inout.ext(its.val, CV); )
	Inout( Wish,  inout.ext(its.val, CV); )		Inout( Body,  inout.ext(its.val, CV); )

	Inout( Fumen,
		inout.container(its.deltas, 131072);	inout.container(its.nodes, 262144);		// Consider "Equal"
		inout.container(its.echoes, 131072);	inout.container(its.wishes, 16777215);	// Wishes
		inout.container(its.hints, 32767);		inout.container(its.wishChilds, 131072);
		inout.container(its.idx, 1024);			inout.value8b(its.val);
	)

	struct A4CONF {
		static constexpr bool			 CheckAdapterErrors = false, CheckDataErrors = false;
		static constexpr EndiannessType  Endianness = DefaultConfig::Endianness;
	};
	using A4Encoder = Serializer< OutputBufferAdapter< std::vector<uint8_t>, A4CONF > >;
	using A4Decoder = Deserializer< InputBufferAdapter<const uint8_t*, A4CONF> >;
}

/* Inout APIs */
int Ar::LoadArf(lua_State* L) {
	/* Usage:
	 * local before, objcnt, wgo_req, hgo_req, ego_req = Arf4.LoadArf(path, is_auto, [proof])
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
			else return free(pBuf), fclose(pF), 0;
		else return 0;

	// Use Proof to Decrypt
	if( size_t proofSize;  lua_type(L, 3) == LUA_TSTRING ) {
		const auto proofStr = (const uint8_t*)lua_tolstring(L, 3, &proofSize);

		uint8_t proofSha256[32];
		dmCrypt::HashSha256( proofStr, (uint32_t)proofSize, proofSha256 );
		Decrypt(dmCrypt::ALGORITHM_XTEA, pBuf, bSize, proofSha256+16, 16);
		Decrypt(dmCrypt::ALGORITHM_XTEA, pBuf, bSize, proofSha256+8, 16);
		Decrypt(dmCrypt::ALGORITHM_XTEA, pBuf, bSize, proofSha256, 16);
	}

	// Decode & Return
	if( auto D = bitsery::A4Decoder(pBuf, bSize);  D.adapter().error() != bitsery::ReaderError::NoError )
		return free(pBuf), 0;
	else
		D.object( Arf = {} );   // Lazy clear only when the buffer is loaded successfully
	#ifndef AR_BUILD_VIEWER
		Arf.isAuto = lua_toboolean(L, 2),			Arf.maxDt = (InputDelta>63 ? 63 : InputDelta) + 37,
													Arf.minDt = Arf.maxDt - 74;
	#endif

	return lua_pushinteger(L, Arf.before),			lua_pushinteger(L, Arf.objectCount),
		   lua_pushinteger(L, Arf.wgoRequired),		lua_pushinteger(L, Arf.hgoRequired),
		   lua_pushinteger(L, Arf.egoRequired),		free(pBuf), 5;
}

int Ar::ExportArf(lua_State* L) {
	/* Usage:
	 * local str_or_nil = Arf4.ExportArf([proof])
	 */
	for( auto& wish : Arf.wishes )
		wish.nIndex = 0, wish.cIndex = 0;

	std::vector<uint8_t> buf;
	auto E = bitsery::A4Encoder(buf);
	E.object(Arf);

	if( const size_t bufSize = ( E.adapter().flush(), E.adapter().writtenBytesCount() );  bufSize ) {
		if( size_t proofSize;  lua_type(L, 1) == LUA_TSTRING ) {
			const auto proofStr = (const uint8_t*)lua_tolstring(L, 1, &proofSize);

			uint8_t proofSha256[32];
			dmCrypt::HashSha256( proofStr, (uint32_t)proofSize, proofSha256 );
			Encrypt(dmCrypt::ALGORITHM_XTEA, &buf[0], bufSize, proofSha256, 16);
			Encrypt(dmCrypt::ALGORITHM_XTEA, &buf[0], bufSize, proofSha256+8, 16);
			Encrypt(dmCrypt::ALGORITHM_XTEA, &buf[0], bufSize, proofSha256+16, 16);
		}
		return lua_pushlstring(L, (char*)&buf[0], bufSize), 1;
	}	return 0;
}

#ifdef AR_BUILD_VIEWER
int Ar::GetFileMtime(lua_State* L) noexcept {
	/* Usage:
	 * local modtime_or_nil = Arf4.GetFileMtime(path)
	 */
	if( struct stat S;  stat( luaL_checkstring(L,1), &S ) == 0 )
		return lua_pushinteger(L, S.st_mtime), 1;
	return 0;
}
#endif