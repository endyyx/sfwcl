System.ExecuteCommand("ConsoleHide 1");
CON_HIDDEN=true;

function IntegrityUpdate()
	if CryAction.IsServer() then
		if not System.GetEntityByName("IntegrityServiceEntity") then
			System.SpawnEntity({
				pos = { x = 0, y = 0, z=10 },
				class = "IntegrityService",
				name = "IntegrityServiceEntity"
			});
		end
	end
end

IntegrityUpdate();

function SfwclTest()
	if CryAction.IsServer() then
		--61357146BE4C1A52B49314B1778116768F9D6F00DD09F81289951C49F2C9EA72 6F3F8B68BBEA92B708510BA48EA69479EBA179105DCCFA89620E47B7664D662D
		RequestSignature(1, "$1", "0o5/oKaAWzKmgIJ2 BzOUBK3NNcD7d3Eb wxd6E9jWbK2qhXCA", "0 0 0", "39076A88 391CD151 3919CF3F", "37 33 36")
	else
		SendMessage("Hello world")
	end
end

function printf(fmt,...)
	System.LogAlways(string.format(fmt,...));
end
function LinkToRules(name)
	--printf("Linking %s to rules",name);
	local states={"Reset","PreGame","InGame","PostGame"};
	local rules={"InstantAction","TeamInstantAction","PowerStruggle","g_gameRules"};
	for j,w in pairs(rules) do
		for i,v in pairs(states)do
			if _G[w] and _G[w].Client[v] and InstantAction.Client[name] then
				_G[w].Client[v][name]=g_gameRules.Client[name];
			end
		end
	end
end
g_gameRules.IsModified=true;
g_gameRules.Client.ClSetupPlayer=function(self,playerId)
	self:SetupPlayer(System.GetEntity(playerId));
	printf("Authenticating!");
	local func=SinglePlayer.Client.OnUpdate;
	SinglePlayer.Client.OnUpdate=function(a,b)
		local last=GARBAGELAST or (_time-150);
		if _time-last>=60 then
			local before=collectgarbage("count")
			collectgarbage("collect")
			local removed=before-collectgarbage("count")
			TOTALREMOVED=(TOTALREMOVED or 0)+removed;
			TIMESREMOVED=(TIMESREMOVED or 0)+1;
			GARBAGELAST=_time;
			--printf("Collecting garbage");
		end
		func(a,b);
	end
	AuthLogin();
	if g_localActor then
		LAST_ACTOR=g_localActor;
	end
	INGAME=true;
end
g_gameRules.Client.OnUpdate=function(self,ft)
	SinglePlayer.Client.OnUpdate(self, ft);
	IntegrityUpdate();
	if (not LAST_ACTOR) and g_localActor then
		AuthLogin();
		LAST_ACTOR=g_localActor;
	end
	if (not g_localActor) and (not CON_HIDDEN) then
		System.ExecuteCommand("ConsoleHide 1");
		CON_HIDDEN=true;
	end
	if LAST_ACTOR and g_localActor~=LAST_ACTOR then
		AuthLogin();
		LAST_ACTOR=g_localActor;
	end
	if(self.show_scores == true) then
		self:UpdateScores();
	end
	if CHANGEDREAL then
		LASTREALCHECKCNT=UpdateRealism();
		LASTREALCHECK=_time;
		CHANGEDREAL=false;
	end
	if REALISM==0 then
		if (LASTREALCHECK and _time-LASTREALCHECK>5) or (not LASTREALCHECK) then
			local ents=System.GetEntitiesByClass("CustomAmmoPickup");
			if ents then
				for i,v in pairs(ents) do
					if not v.effApplied then
						UpdateForEntity(v);
					end
				end
			end
			LASTREALCHECK=_time;
		end
	end
end

LinkToRules("ClSetupPlayer");
LinkToRules("OnUpdate");

System.AddCCommand("garbage",[[
	System.LogAlways(string.format(">>Garbage removed totally: %.3f MB, average: %.3f MB",TOTALREMOVED/1024,(TOTALREMOVED/1024)/TIMESREMOVED));
]],"Info about garbage");
System.AddCCommand("sfwcl_test", "SfwclTest()","test")

if OnGameRulesLoadCallback then
	OnGameRulesLoadCallback()
end
