using UnrealBuildTool;

public class ProjectLogos : ModuleRules
{
	public ProjectLogos(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
        		{
        			"Core",
        			"CoreUObject",
        			"Engine",
        			"EnhancedInput",
        			"GameplayAbilities",
        			"GameplayTags",
        			"GameplayTasks",
        			"InputCore",
        			"Mover"
        		});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
