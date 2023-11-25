project "Premake"
	kind "Utility"

	targetdir ("%{wks.location}/Build/" .. OutputDir)
	objdir ("%{wks.location}/Build/Objects/" .. OutputDir .. "/%{prj.name}")

	vpaths
	{
		["Kr"] = { "../Kr/**" },
		["Karma"] = { "../../Karma/**" },
		["Premake"] = { "**" },
	}

	files
	{
		"%{wks.location}/**premake5.lua"
	}

	postbuildmessage "Regenerating project files with Premake5!"
	postbuildcommands
	{
		"\"%{prj.location}premake5\" %{_ACTION} --file=\"%{wks.location}premake5.lua\""
	}