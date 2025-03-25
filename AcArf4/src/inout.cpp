//  Arf4 Inout  //
#include <Arf4.h>
#include <bitsery/bitsery.h>
#include <bitsery/traits/adapter_buffer.h>
#include <bitsery/traits/container_vector.h>
#include <bitsery/traits/compact_value.h>
#include <dmsdk/dlib/crypt.h>

/* Bitsery Settings */
#define Inout(TYPE, DETAILS)	template<typename S> void serialize(S& s, Arf4:: TYPE &its) {			   \
			  s.enableBitPacking( [&its](typename S::BPEnabledType& inout) { DETAILS ; } ); }
namespace bitsery {
	static constexpr auto CV = ext::CompactValueAsObject{};
	Inout( Info,  inout.ext(its.val, CV); )			Inout( Wish,  inout.ext(its.val, CV); )
	Inout( Hint,  inout.ext(its.val, CV); )			Inout( Echo,  inout.ext(its.val, CV); )
	Inout( Point, inout.ext(its.val, CV); )			Inout( Child, inout.ext(its.val, CV); )
	Inout( Delta, inout.ext(its.val, CV); )

	Inout( Fumen,
		inout.container(its.deltas, 8191);			inout.container(its.nodes, 32767);   // Consider "Equal"
		inout.container(its.echoes, 32767);			inout.container(its.wishes, 65535);  // Wishes
		inout.container(its.hints, 32767);			inout.container(its.wishChilds, 32767);
		inout.container(its.hIdx, 1024);			inout.container(its.wIdx, 512);
		inout.container(its.eIdx, 1024);			inout.value8b(its.val);
	)

	struct Arf4Config {
		static constexpr bool			 CheckAdapterErrors = false, CheckDataErrors = false;
		static constexpr EndiannessType  Endianness = DefaultConfig::Endianness;
	};
	using GetArf4Encoder = Serializer< OutputBufferAdapter< std::vector<uint8_t>, Arf4Config > >;
	using GetArf4Decoder = Deserializer< InputBufferAdapter<const uint8_t*, Arf4Config> >;
}

/* Inout APIs */
int Ar::LoadArf(lua_State* L) {
	/* Usage:
	 * local before_or_false, objcnt, wgo_req, hgo_req, ego_req = Arf4.LoadArf(path, is_auto, [proof])
	 */
	struct PseudoContext {
		dmConfigFile::HConfig  pConfig;
		dmResource::HFactory   pFactory;
	};
	lua_pushnumber(L, 2744634527);									// Args -> hash"__script_context"
	lua_gettable(L, LUA_GLOBALSINDEX);								// Args -> context
	const auto pContext = (PseudoContext*)lua_touserdata(L, -1);	lua_pop(L, 1);
	const auto path = luaL_checkstring(L, 1);

	// Acquire Buffer
	uint8_t* pBuf;													// free() this.
	uint32_t bufSize;
	if( const auto loadResult = dmResource::GetRaw(pContext->pFactory, path, (void**)&pBuf, &bufSize);
		loadResult != dmResource::RESULT_OK
	) {
		FILE* pFile = fopen(path, "rb");							// Open
		if( pFile == nullptr )
			return lua_pushboolean(L, false), 1;

		(void)fseek(pFile, 0, SEEK_END);							// Size
		bufSize = ftell(pFile);
		(void)fseek(pFile, 0, SEEK_SET);

		pBuf = (uint8_t*)malloc(bufSize);							// Copying
		if( fread( pBuf, 1, bufSize, pFile ) != bufSize )
			return lua_pushboolean(L, false), free(pBuf), fclose(pFile), 1;
		(void)fclose(pFile);
	}

	// Use Proof to Decrypt
	if( size_t proofSize;  lua_type(L, 3) == LUA_TSTRING ) {
		const auto proofStr = (const uint8_t*)lua_tolstring(L, 3, &proofSize);

		uint8_t proofSha256[32];
		dmCrypt::HashSha256( proofStr, (uint32_t)proofSize, proofSha256 );
		Decrypt(dmCrypt::ALGORITHM_XTEA, pBuf, bufSize, proofSha256+16, 16);
		Decrypt(dmCrypt::ALGORITHM_XTEA, pBuf, bufSize, proofSha256+8, 16);
		Decrypt(dmCrypt::ALGORITHM_XTEA, pBuf, bufSize, proofSha256, 16);
	}

	// Decode & Return
	auto decodeState = bitsery::GetArf4Decoder(pBuf, bufSize);
	const bool readError =  decodeState.adapter().error() != bitsery::ReaderError::NoError,
			   desError  = !decodeState.adapter().isCompletedSuccessfully();
	if( readError || desError )
		return lua_pushboolean(L, false), free(pBuf), 1;
	decodeState.object( Arf = {} );   // Lazy clear only when the buffer is loaded successfully.

	Arf.isAuto = lua_toboolean(L, 2);
	Arf.maxDt = (InputDelta>63 ? 63 : InputDelta) + 37;
	Arf.minDt = Arf.maxDt - 74;

	return lua_pushinteger(L, Arf.before),			lua_pushinteger(L, Arf.objectCount),
		   lua_pushinteger(L, Arf.wgoRequired),		lua_pushinteger(L, Arf.hgoRequired),
		   lua_pushinteger(L, Arf.egoRequired),		free(pBuf), 5;
}

#ifdef AR_BUILD_VIEWER
	int Ar::ExportArf(lua_State* L) {
		/* Usage:
		 * local str_or_nil = Arf4.ExportArf([proof])
		 */
		for( auto& wish : Arf.wishes )
			wish.nIndex = 0, wish.cIndex = 0;

		std::vector<uint8_t> buf;
		auto enc = bitsery::GetArf4Encoder(buf);
		enc.object(Arf);

		if( const size_t bufSize = ( enc.adapter().flush(), enc.adapter().writtenBytesCount() ); bufSize ) {
			if( size_t proofSize;  lua_type(L, 1) == LUA_TSTRING ) {
				const auto proofStr = (const uint8_t*)lua_tolstring(L, 1, &proofSize);

				uint8_t proofSha256[32];
				dmCrypt::HashSha256( proofStr, (uint32_t)proofSize, proofSha256 );
				Encrypt(dmCrypt::ALGORITHM_XTEA, &buf[0], bufSize, proofSha256, 16);
				Encrypt(dmCrypt::ALGORITHM_XTEA, &buf[0], bufSize, proofSha256+8, 16);
				Encrypt(dmCrypt::ALGORITHM_XTEA, &buf[0], bufSize, proofSha256+16, 16);
			}
			return lua_pushlstring(L, (char*)&buf[0], bufSize), 1;
		}
		return 0;
	}
#endif