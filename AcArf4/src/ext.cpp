//  Arf4 Ext  //
#include <Arf4.h>

Arf4::Fumen		Arf;
int8_t			InputDelta;
float			PlayerSpeed = 6;

static constexpr luaL_Reg Arf4Lib[] = {
	#ifdef AR_BUILD_VIEWER
		{"NewBuild", Ar::NewBuild},
		{"SetDelta", Ar::SetDelta},
		{"NewVerse", Ar::NewVerse},
		{"NewWish", Ar::NewWish},
		{"NewHint", Ar::NewHint},
		{"NewEcho", Ar::NewEcho},
		{"NewChild", Ar::NewChild},
		{"NewHelper", Ar::NewHelper},
		{"SinceTone", Ar::SinceTone},
		{"OrganizeArf", Ar::OrganizeArf},
		{"ExportArf", Ar::ExportArf},
		{"BarToMs", Ar::BarToMs},
		{"Mirror", Ar::Mirror},
	#else
		{"JudgeArf", Ar::JudgeArf},
		{"SetJudgeZone", Ar::SetJudgeZone},
		{"SetInputDelta", Ar::SetInputDelta},
		{"GetJudgeStat", Ar::GetJudgeStat},
		{"SetSpeed", Ar::SetSpeed},
		{"Ease", Ar::Ease},
	#endif
		{"LoadArf", Ar::LoadArf},
		{"UpdateArf", Ar::UpdateArf},
		{"SetCam", Ar::SetCam},
		{nullptr, nullptr}
};

static dmExtension::Result Arf4Init(dmExtension::Params* p) {
	luaL_register(p->m_L, "Arf4", Arf4Lib), lua_pop(p->m_L, 1);
	return dmExtension::RESULT_OK;
}
static dmExtension::Result Arf4OK(dmExtension::Params*) {
	return dmExtension::RESULT_OK;
}
static dmExtension::Result Arf4APPOK(dmExtension::AppParams*) {
	return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(AcArf4, "AcArf4", Arf4APPOK, Arf4APPOK, Arf4Init, nullptr, nullptr, Arf4OK)