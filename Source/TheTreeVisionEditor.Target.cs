using UnrealBuildTool;
using System.Collections.Generic;

public class TheTreeVisionEditorTarget : TargetRules
{
	public TheTreeVisionEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("TheTreeVision");
	}
}
