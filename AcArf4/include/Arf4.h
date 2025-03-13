//  Aerials Fumen Player v4  //
#pragma once
#include <dmsdk/sdk.h>
#include <vector>

inline const dmVMath::Vector3
	AnimTint[]{ {1, 1, 1}, {0.3125, 0.5625, 0.63671875}, {0.63671875, 0.38671875, 0.3125} },
	HintEarly	{0.37675, 0.67815, 0.767628125},
	HintLate	{0.767628125, 0.466228125, 0.37675},
	HintHit		{0.88, 0.88, 0.88};

namespace Arf4 {
	#define	Ar32(...)  using i32 = int32_t; using u32 = uint32_t;  union{ struct{__VA_ARGS__;}; u32 val; };
	#define Ar64(...)  using i64 = int64_t; using u64 = uint64_t;  union{ struct{__VA_ARGS__;}; u64 val; };

	enum  TableIndex : uint8_t  {	   WGO = 3, HGO, EGO, EH, AL, AR, WTINT, HTINT, ETINT, EHTINT, ATINT  };
	enum  EaseType   : uint8_t  {   STATIC = 0, LINEAR, INSINE, OUTSINE									  };
	enum  Status     : uint8_t  {  NJUDGED = 0, SPECIAL, NJUDGED_LIT, SPECIAL_LIT, HIT, HIT_LIT, LOST,
								  SPECIAL_LOST, EARLY, EARLY_LIT, LATE, LATE_LIT						  };
/* Wish */
	struct Point {
		Ar64(																// cdx: [-64,64) 1/8
			i64  cdx:9, cdy:8;				u64  ease:2, ms:20;				// cdy: [-32,32) 1/8
			u64  radius:6;					i64  deg:18;					// rad  [0,16) 1/4
		)																	// deg  [-1024,1023) recommended
	};
	struct Child {
		Ar64(
			u64  radius:5, initLoop:6;		i64  deltaLoop:5;
			u64  zDt:33;													// zDt = ms * v
		)
	};
	struct Wish {
		Ar64(
			u64  nCount:6,  nSince:15;		u64  withDt:1;
			u64  cCount:10, cSince:15;		u64  isSpecial:1;
			u64  nIndex:6,  cIndex:10;
		)
	};

/* Object */
	struct Hint {
		Ar64(
			i64  cdx:9, cdy:8;				u64  ms:20, status:4;
			i64  deltaMs:8;
		)
	};
	struct Echo {
		Ar64(																// radius	 [0,8)  1/4
			i64  cdx:9, cdy:8;				u64  ms:20, status:3;			// initLoop  [0,1)  1/64
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
	struct Duo {
		Ar64(
			union				{  float a;  struct{ uint32_t am:23, ae:8, as:1; };  };
			union				{  float b;  struct{ uint32_t bm:23, be:8, bs:1; };  };
		)
	};
	struct Info { Ar32(u32 c:10, f:22) };

/* Fumen */
	struct Fumen {
		std::vector<Point>		nodes;
		std::vector<Delta>		deltas;										// [0] Index
		std::vector<Child>		wishChilds;
		std::vector<Info>		wIdx, hIdx, eIdx;
		std::vector<Wish>		wishes;
		std::vector<Echo>		echoes;
		std::vector<Hint>		hints;
		/*------------------------*/
		Ar64(
			uint64_t			before:20, objectCount:16;
			uint64_t			wgoRequired:10, hgoRequired:9, egoRequired:9;
		)
		//------------------------//
		int64_t					minDt:8, maxDt:8;
		uint64_t				msTime:20, judgeRange:7 = 37;
		uint64_t				sHit:6, hHit:15, eHit:15, early:15, late:15, lost:16;
		uint64_t				isAuto:1, isAnyX:1, isAnyY:1;
		/*------------------------*/
		float					xScale = 112.5/8, yScale = xScale;
		float					xDelta, cSpeed = 1;							// cS ∈ [0,1.25]  pS ∈ [0.5,10]
	};
}
extern  float					PlayerSpeed;								// Finally  (pS*cS + 11) / 1500
extern  int8_t					InputDelta;
extern  uint64_t				UsysTime;
extern  Arf4::Fumen				Arf;

namespace Ar {
	using namespace Arf4;

	/* Build */
	int  NewBuild(lua_State* L);
	int  NewDeltaGroup(lua_State* L) noexcept;
	int  DeltaTone(lua_State* L) noexcept;
	int  NewVerse(lua_State*) noexcept;
	int  MirrorLR(lua_State*) noexcept;
	int  MirrorUD(lua_State*) noexcept;
	int  NewHelper(lua_State* L) noexcept;
	int  NewChild(lua_State* L);
	int  NewWish(lua_State* L);
	int  NewHint(lua_State* L);
	int  NewEcho(lua_State* L);
	int  BarToMs(lua_State* L) noexcept;
	int  Move(lua_State* L) noexcept;

	/* Internal */
  float  Eased(double, uint8_t) noexcept;
   void  JudgeArfSweep() noexcept;
	Duo  CosSin(Duo) noexcept;

	/* Operation */
	int  LoadArf(lua_State* L);
	int  ExportArf(lua_State* L);
	int  OrganizeArf(lua_State* L) noexcept;
	int  UpdateArf(lua_State* L) noexcept;
	int  JudgeArf(lua_State* L) noexcept;

	/* Runtime Utils */
	int  Ease(lua_State* L) noexcept;
	int  SetCam(lua_State* L) noexcept;
	int  SetSpeed(lua_State* L) noexcept;
	int  SetJudgeZone(lua_State* L) noexcept;
	int  GetJudgeStat(lua_State* L) noexcept;
	int  SetInputDelta(lua_State* L) noexcept;
	int  TransformStr(lua_State* L);
}