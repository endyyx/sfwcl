System.ExecuteCommand("ConsoleHide 1");
CON_HIDDEN=true;
if ver==5767 then
	Script.LoadScript("scripts/gamerules/instantaction100.lua", 1, 1);
elseif ver==6156 then
	Script.LoadScript("scripts/gamerules/instantaction121.lua", 1, 1);
elseif ver==6729 then
	Script.LoadScript("scripts/gamerules/instantaction150.lua", 1, 1);
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
				_G[w].Client[v][name]=InstantAction.Client[name];
			end
		end
	end
end
InstantAction.IsModified=true;
InstantAction.Client.ClSetupPlayer=function(self,playerId)
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
InstantAction.Client.OnUpdate=function(self,ft)
	SinglePlayer.Client.OnUpdate(self, ft);
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
	--[[if g_localActor then
		if g_localActor.actorStats.inAir>0.001 then
			local vec=g_localActor:GetBoneDir("Bip01 head");
			local pos=g_localActor:GetPos();
			if g_localActor.lastX then
				local dx,dy=pos.x-g_localActor.lastX,pos.y-g_localActor.lastY;
				local speed=math.sqrt(dx*dx+dy*dy);
				if vec.z<0 then
					vec.z=-vec.z;
					local force=g_localActor:GetMass()*9.81*speed*ft;
					g_localActor:AddImpulse(-1,g_localActor:GetCenterOfMassPos(),{0,0,1},force,1);
					--if speed>1 then
					--	printf("Push %f up, speed: %f km/h",force,speed);
					--end
				end
			end
			g_localActor.lastX=pos.x;
			g_localActor.lastY=pos.y;
		end
	end--]]
end

LinkToRules("ClSetupPlayer");
LinkToRules("OnUpdate");

System.AddCCommand("garbage",[[
	System.LogAlways(string.format(">>Garbage removed totally: %.3f MB, average: %.3f MB",TOTALREMOVED/1024,(TOTALREMOVED/1024)/TIMESREMOVED));
]],"Info about garbage");
