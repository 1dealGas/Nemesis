//  Aerials Fumen Player v4  //
#pragma once
#include <dmsdk/sdk.h>
#include <vector>

namespace Arf4 {
	#define Ar64(...)  using i64 = int64_t; using u64 = uint64_t;  union{ struct{__VA_ARGS__;}; u64 val; };
	enum  /*   Status   */ {  NJUDGED = 0, SPECIAL, NJUDGED_LIT, SPECIAL_LIT, HIT, HIT_EC, HLIT, HLIT_EC };
	enum  /*  EaseType  */ {   STATIC = 0, LINEAR, INSINE, OUTSINE					/* EC: Edge Cases */ };
/* Wish */
	struct Point {
		Ar64(																// cdx  (-32,32) 1/16
			i64  cdx:10, cdy:7;				u64  ease:2, ms:20;				// cdy  (-4,4) 1/16
			u64  radius:6;					i64  deg:11;					// rad  [0,16) 1/4
		)																	// deg  [-1024,1024) 1/1
	};
	struct Child {
		Ar64(
			u64  radius:5, initLoop:6;		i64  deltaLoop:5;
			u64  ms:20;
		)
	};
	struct Wish {
		Ar64(																// Limits:
			u64  nType:1, isSpecial:1;		u64  cType:1, withCs:1;			// Node	  [1] 4096	[T] (262144)
			u64  nSince:18, cSince:17;		u64  nIndex:12, cIndex:13;		// Child  [1] 8192	[T] (131072)
		)																	// Wish   [X] 1023	[T] 16777215
	};

/* Object */																// Limits for Objects:
	struct Body {															// [X] 511  [T] 32767
		Ar64(
			i64  cdx:10, cdy:7;				u64  ms:20, status:3;			// radius	 [0,8)  1/4
			u64  radius:5, initLoop:6;		i64  deltaLoop:5, deltaMs:8;	// initLoop  [0,1)  1/64
		)																	// deltaLoop (-2,2) 1/8
	};

/* Misc */
	struct Index {
		Ar64(
			u64  hSince:15, eSince:17;										// [OI] 1024ms, ABCDEF···
			u64  wSince:24;													// [WI] 2048ms, ABC000···
		)
	};
	struct Duo {
		Ar64(
			union  {  float a;  struct{ uint32_t am:23, ae:8, as:1; };  };
			union  {  float b;  struct{ uint32_t bm:23, be:8, bs:1; };  struct{ uint32_t es:2, ms:30; };  };
		)
	};

/* Fumen */
	struct Fumen {
		std::vector<Index>		idx;
		std::vector<Point>		nodes;										// [Dif] 19+ (val > 0)
		std::vector<Child>		wishChilds;									// [Dif] zDt (val > 0)
		std::vector<Wish>		wishes;
		std::vector<Body>		hints, echoes;								// [Dif] ms, echo.40+
		Ar64(	uint64_t		before:20, objectCount:16;
				uint64_t		wgoRequired:10, hgoRequired:9, egoRequired:9;	)
		//------------------------//
		int64_t					minDt:8, maxDt:8;
		uint64_t				msTime:20, judgeRange:7 = 37;
		uint64_t				sHit:6, hHit:15, eHit:15, early:15, late:15, lost:16;
		uint64_t				isAuto:1, isAnyX:1, isAnyY:1;
		float					xScale = 112.5 / 16, yScale = 7.03125;
		float					xDelta, cSpeed = 1;
	};
}
extern  float					PlayerSpeed;								// [0.5,15]
extern  int8_t					InputDelta;
extern  Arf4::Fumen				Arf;

namespace Ar {
	using namespace Arf4;
   inline constexpr auto f4 = "Arf4", LGC = "__gc";

	/* Build */
	int  NewBuild(lua_State*) noexcept;
	int  NewWish(lua_State*) noexcept;
	int  NewHint(lua_State*) noexcept;
	int  NewEcho(lua_State*) noexcept;
	int  NewChild(lua_State*) noexcept;
	int  NewHelper(lua_State*) noexcept;
	int  SinceTone(lua_State*) noexcept;
	int  ConvTime(lua_State*) noexcept;

	/* Internal */
  float  Eased(float, uint8_t) noexcept;
   void  JudgeArfSweep() noexcept;
	Duo  CosSin(Duo) noexcept;

	/* Operation */
	int  LoadArf(lua_State*);
	int  ExportArf(lua_State*);
	int  OrganizeArf(lua_State*) noexcept;
	int  UpdateArf(lua_State*) noexcept;
	int  JudgeArf(lua_State*) noexcept;

	/* Runtime Utils */
	int  Bind(lua_State*) noexcept;
	int  SetCam(lua_State*) noexcept;
	int  SetOptions(lua_State*) noexcept;
	int  SetJudgeZone(lua_State*) noexcept;
	int  SetJudgeStat(lua_State*) noexcept;
	int  GetJudgeStat(lua_State*) noexcept;
	int  GetFileMtime(lua_State*) noexcept;
	int  GetCosSin(lua_State*) noexcept;
	int  NewSeries(lua_State*) noexcept;
	int  Ease(lua_State*) noexcept;
}