//  Aerials Fumen Player v4  //
#pragma once
#include <dmsdk/sdk.h>
#include <vector>

namespace Arf4 {
	#define Ar64(...)  using i64 = int64_t; using u64 = uint64_t;  union{ struct{__VA_ARGS__;}; u64 val; };
	enum  /* TableIndex */ {	  WGO = 3, HGO, EGO, EH, AL, AR, WTINT, HTINT, ETINT, EHTINT, ATINT  };
	enum  /*  EaseType  */ {   STATIC = 0, LINEAR, INSINE, OUTSINE									 };
	enum  /*   Status   */ {  NJUDGED = 0, SPECIAL, NJUDGED_LIT, SPECIAL_LIT, HIT, HIT_ES, HIT_LIT,
							   HIT_LIT_ES, EARLY, LATE, EARLY_LIT, LATE_LIT, LOST					 };
/* Wish */
	struct Point {
		Ar64(																// cdx: [-32,32) 1/16
			i64  cdx:10, cdy:7;				u64  ease:2, ms:20;				// cdy: [-8,8) 1/8
			u64  radius:6;					i64  deg:11;					// rad  [0,16) 1/4
		)																	// deg  [-1024,1023) 1/1
	};
	struct Child {
		Ar64(
			u64  radius:5, initLoop:6;		i64  deltaLoop:5;
			u64  zDt:33;													// zDt = ms * v
		)
	};
	struct Wish {
		Ar64(																// Limits:
			u64  nType:1, isSpecial:1;		u64  cType:1, withDt:1;			// Node	  [1] 4096	[T] (262144)
			u64  nSince:18, cSince:17;		u64  nIndex:12, cIndex:13;		// Child  [1] 8192	[T] (131072)
		)																	// Wish   [X] 1023	[T] 16777215
	};

/* Object */
	struct Hint {
		Ar64(
			i64  cdx:10, cdy:7;				u64  ms:20, status:4;			// Limits for Objects:
			i64  deltaMs:8;													// [X] 511  [T] 32767
		)
	};
	struct Echo {
		Ar64(																// radius	 [0,8)  1/4
			i64  cdx:10, cdy:7;				u64  ms:20, status:3;			// initLoop  [0,1)  1/64
			u64  radius:5, initLoop:6;		i64  deltaLoop:5, deltaMs:8;	// deltaLoop (-2,2) 1/8
		)
	};

/* Misc */
	struct Delta {
		Ar64(																// v [-8,8)      1/1024
			u64  t:18;						u64  absV:13;					// t [0,1048576) 4/1ms
			u64  base:33;													// base = t * v
		)
	};
	struct Index {
		Ar64(
			u64  hSince:15 = 0,  eSince:17 = 0;								// [OI] 1024ms, ABCDEF···
			u64  wSince:24 = 0;												// [WI] 2048ms, AABBCC···
		)
	};
	struct Duo {
		Ar64(
			union				{  float a;  struct{ uint32_t am:23, ae:8, as:1; };  };
			union				{  float b;  struct{ uint32_t bm:23, be:8, bs:1; };  };
		)
	};

/* Fumen */
	struct Fumen {
		std::vector<Index>		idx;
		std::vector<Point>		nodes;
		std::vector<Delta>		deltas;										// [0] Index	[Max] 1+8190
		std::vector<Child>		wishChilds;
		std::vector<Wish>		wishes;
		std::vector<Echo>		echoes;
		std::vector<Hint>		hints;
		Ar64(	uint64_t		before:20, objectCount:16;
				uint64_t		wgoRequired:10, hgoRequired:9, egoRequired:9;	)
		//------------------------//
		int64_t					minDt:8, maxDt:8;
		uint64_t				msTime:20, judgeRange:7 = 37;
		uint64_t				sHit:6, hHit:15, eHit:15, early:15, late:15, lost:16;
		uint64_t				isAuto:1, isAnyX:1, isAnyY:1;
		float					xScale = 112.5 / 16, yScale = 112.5 / 8;
		float					xDelta, cSpeed = 1;							// cS ∈ [0,1.25]  pS ∈ [0.5,10]
	};
}
extern  float					PlayerSpeed;								// Finally  (pS*cS + 11) / 1500
extern  int8_t					InputDelta;
extern  Arf4::Fumen				Arf;

namespace Ar {
	using namespace Arf4;

	/* Build */
	int  NewBuild(lua_State*) noexcept;
	int  SetDelta(lua_State*) noexcept;
	int  NewVerse(lua_State*) noexcept;
	int  NewWish(lua_State*) noexcept;
	int  NewHint(lua_State*) noexcept;
	int  NewEcho(lua_State*) noexcept;
	int  NewChild(lua_State*) noexcept;
	int  NewHelper(lua_State*) noexcept;
	int  SinceTone(lua_State*) noexcept;
	int  BarToMs(lua_State*) noexcept;
	int  Mirror(lua_State*) noexcept;

	/* Internal */
  float  Eased(double, uint8_t) noexcept;
   void  JudgeArfSweep() noexcept;
	Duo  CosSin(Duo) noexcept;

	/* Operation */
	int  LoadArf(lua_State*);
	int  ExportArf(lua_State*);
	int  OrganizeArf(lua_State*) noexcept;
	int  UpdateArf(lua_State*) noexcept;
	int  JudgeArf(lua_State*) noexcept;

	/* Runtime Utils */
	int  Ease(lua_State*) noexcept;
	int  SetCam(lua_State*) noexcept;
	int  SetSpeed(lua_State*) noexcept;
	int  GetJudgeStat(lua_State*) noexcept;
	int  SetJudgeStat(lua_State*) noexcept;
	int  SetJudgeZone(lua_State*) noexcept;
	int  SetInputDelta(lua_State*) noexcept;
}