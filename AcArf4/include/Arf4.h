//  Aerials Fumen Player v4  //
#pragma once
#include <dmsdk/sdk.h>

namespace Ar {
	#define A4(TP,...)	union TP  { struct{__VA_ARGS__};  u64 val = 0; };
						using i64 = int64_t;		using u64 = uint64_t;
	enum  /*   Status   */ {  NJUDGED = 0, SPECIAL, NJUDGED_LIT, SPECIAL_LIT, HIT, HIT_EC, HLIT, HLIT_EC };
	enum  /*  EaseType  */ {   STATIC = 0, LINEAR, INSINE, OUTSINE					/* EC: Edge Cases */ };

/* Wish */
	A4( Point,																// cdx  (-32,32) 1/16
		i64  cdx:10, cdy:7;				u64  ease:2, ms:20;					// cdy  (-4,4) 1/16
		u64  radius:6;					i64  deg:11;					)	// rad  [0,16) 1/4
																			// deg  [-1024,1024) 1/1
	A4( Child,
		u64  radius:5, initLoop:6;		i64  deltaLoop:5;
		u64  ms:20;														)

	A4( Wish,																// Limits:
		u64  nType:1, isSpecial:1;		u64  cType:1, withCs:1;				// Node	  [1] 4096	[T] (262144)
		u64  nSince:18, cSince:17;		u64  nIndex:12, cIndex:13;		)	// Child  [1] 8192	[T] (131072)
																			// Wish   [X] 1023	[T] 1048576
/* Data */																	//		  [O] 511	[T] 65535
	A4( Body,
		i64  cdx:10, cdy:7;				u64  ms:20, status:3;				// radius	 [0,8)  1/4
		u64  radius:5, initLoop:6;		i64  deltaLoop:5, deltaMs:8;	)	// initLoop  [0,1)  1/64
																			// deltaLoop (-2,2) 1/8
	A4( Index,
		u64  hSince:15, eSince:17;											// [OI] 1024ms, ABCDEF···
		u64  wSince:20;													)	// [WI] 2048ms, ABC000···

	A4( Duo,
		union  {  float a;  struct{ uint32_t am:23, ae:+8, as:1; };	};
		union  {  float b;  struct{ uint32_t ms:20, es:11, bs:1; };	};	)

/* Fumen */
	struct Fumen {															// ( Whole buffer at Arf.idx )
		Index  *idx;														// Node   [Dif] ms (val > 0)
		Point  *node;			Wish  *wish;								// Child  [Dif] ms (val > 0)
		Child  *child;			Body  *hint, *echo, *ceil;					// Object [Dif] ms, echo.40+
		A4(,;	uint64_t		wgoRequired:10, hgoRequired:9, egoRequired:9;
				uint64_t		before:20, objectCount:16;						)
		//------------------------//
		uint64_t				msTime:20, judgeZone:7 = 37;
		uint64_t				vsTime:20, sHit:14, isAnyX:1, isAnyY:1, isAuto:1;
		uint32_t				lost:16, hit:16, early:15, late:15, hintHit:1, echoHit:1;
		float					xScale = 112.5 / 16, yScale = 7.03125,
								xDelta, cSpeed = 1;
	};

/* Build APIs */
	int  NewBuild(lua_State*) noexcept;
	int  NewWish(lua_State*) noexcept;
	int  NewHint(lua_State*) noexcept;
	int  NewEcho(lua_State*) noexcept;
	int  NewChild(lua_State*) noexcept;
	int  NewHelper(lua_State*) noexcept;
	int  SinceTone(lua_State*) noexcept;
	int  ConvTime(lua_State*) noexcept;

/* Operation APIs */
	int  LoadArf(lua_State*) noexcept;
	int  ExportArf(lua_State*) noexcept;
	int  OrganizeArf(lua_State*) noexcept;
	int  UpdateArf(lua_State*) noexcept;
	int  JudgeArf(lua_State*) noexcept;

/* Internal */
  float  Eased(float, uint8_t) noexcept;
   void  JudgeArfSweep() noexcept;
	Duo  CosSin(Duo) noexcept;

/* Runtime Utils */
	int  Bind(lua_State*) noexcept;
	int  SetCam(lua_State*) noexcept;
	int  SetOptions(lua_State*) noexcept;
	int  SetJudgeZone(lua_State*) noexcept;
	int  GetJudgeStat(lua_State*) noexcept;
	int  GetFileMtime(lua_State*) noexcept;
	int  GetCosSin(lua_State*) noexcept;
	int  NewSeries(lua_State*) noexcept;
	int  Ease(lua_State*) noexcept;
}

  extern  float				PlayerSpeed;									// [0.5,15]
  extern  int8_t			InputDelta;
  extern  Ar::Fumen			Arf;