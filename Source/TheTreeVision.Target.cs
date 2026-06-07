using UnrealBuildTool;
using System.Collections.Generic;

public class TheTreeVisionTarget : TargetRules
{
	public TheTreeVisionTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("TheTreeVision");
	}
}
