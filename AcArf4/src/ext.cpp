//  Arf4 Ext  //
#include <Arf4.h>
Arf4::Fumen		Arf;
int8_t			InputDelta;
float			PlayerSpeed = 6;

static int freeSeries(lua_State* L) {
	( (std::vector<Arf4::Duo>*)lua_touserdata(L, 1) ) -> ~vector();
	return 0;
}

static constexpr luaL_Reg Arf4Lib[] = {
	#ifdef AR_BUILD_VIEWER
		{"NewBuild", Ar::NewBuild},
		{"SetDelta", Ar::SetDelta},
		{"NewWish", Ar::NewWish},
		{"NewHint", Ar::NewHint},
		{"NewEcho", Ar::NewEcho},
		{"NewChild", Ar::NewChild},
		{"NewHelper", Ar::NewHelper},
		{"SinceTone", Ar::SinceTone},
		{"OrganizeArf", Ar::OrganizeArf},
		{"GetFileMtime", Ar::GetFileMtime},
		{"ConvTime", Ar::ConvTime},
	#else
		{"JudgeArf", Ar::JudgeArf},
		{"SetOptions", Ar::SetOptions},
		{"SetJudgeZone", Ar::SetJudgeZone},		
		{"SetJudgeStat", Ar::SetJudgeStat},
		{"GetJudgeStat", Ar::GetJudgeStat},
	#endif
		{Ar::LGC, freeSeries},
		{"SetCam", Ar::SetCam},
		{"LoadArf", Ar::LoadArf},
		{"ExportArf", Ar::ExportArf},
		{"UpdateArf", Ar::UpdateArf},
		{"NewSeries", Ar::NewSeries},
		{"CosSin", Ar::GetCosSin},
		{"Ease", Ar::Ease},
	{0,0}
};
static dmExtension::Result Arf4Init(dmExtension::Params* p) {
	return luaL_register(p->m_L, Ar::f4, Arf4Lib),  lua_pop(p->m_L, 1),  dmExtension::RESULT_OK;
}
static dmExtension::Result Arf4OK(dmExtension::Params*) {
	return dmExtension::RESULT_OK;
}
DM_DECLARE_EXTENSION(AcArf4, Ar::f4, nullptr, nullptr, Arf4Init, nullptr, nullptr, Arf4OK)