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
				_G[w].Client[name]=g_gameRules.Client[name];
			end
		end
	end
end
g_gameRules.IsModified=true;
g_gameRules.Client.ClSetupPlayer=function(self,playerId)
	self:SetupPlayer(System.GetEntity(playerId));
	AuthLogin();
	if g_localActor then
		LAST_ACTOR=g_localActor;
	end
	INGAME=true;
end

LinkToRules("ClSetupPlayer");

System.AddCCommand("garbage",[[
	System.LogAlways(string.format(">>Garbage removed totally: %.3f MB, average: %.3f MB",TOTALREMOVED/1024,(TOTALREMOVED/1024)/TIMESREMOVED));
]],"Info about garbage");

if not OldSP then OldSP = SinglePlayer.Client.OnUpdate end

function HookedStartWorking(self, entityId, workName)
	if workName:sub(1, 1) == "@" and RPC ~= nil then
		if ALLOW_EXPERIMENTAL then
			System.Log("Received RPC call: " .. workName)
		end
		local what = json.decode(workName:sub(2))
		if what and what.method then
			if ALLOW_EXPERIMENTAL then
				System.Log("Resolved RPC method: " .. what.method)
			end
			local method = RPC[what.method]
			if RPC_STATE then
				if what.class then
					method = RPC[what.class].method
					if method then
						_pcall(method, RPC[what.class], what.params, what.id)
					end
				elseif method then
					_pcall(method, what.params, what.id)
				end
			end
		end
	else
		self.work_type=workName;
		self.work_name="@ui_work_"..workName;
		HUD.SetProgressBar(true, 0, self.work_name);
	end
end

SinglePlayer.Client.OnUpdate = function(self, dt)
	OldSP(self, dt)
	if not (g_gameRules.class == "InstantAction" or g_gameRules.class == "PowerStruggle") then
		return;
	end
	
	if g_gameRules.Client.ClStartWorking ~= HookedStartWorking then
		--printf("Hooked ClStartWorking")
		g_gameRules.Client.ClStartWorking = HookedStartWorking
		--LinkToRules("ClStartWorking")
	end
	
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
	
	for i,params in pairs(ActiveAnims) do
		if params then
			local ent = params.entity;
			if not ent then params.entity = System.GetEntityByName(params.name); ent = params.entity; end
			if ent then
				local dur = _time - params.start;
				
				if params.pos then
					ent:SetWorldPos(lerp(params.pos.from, params.pos.to, dur / params.duration))
				end
				
				if params.scale then
					ent:SetScale(lerp(params.scale.from, params.scale.to, dur / params.duration))
				end
				
				if dur >= params.duration then
					ActiveAnims[i] = nil
				end
			end
		end
	end
	
	for i,v in pairs(ActiveFx) do
		if v~=nil then
			System.SetScreenFx(i, v)
		end
	end
	
	if (_LASTREALCHECK and _time-_LASTREALCHECK>5) or (not _LASTREALCHECK) then
		local ents=System.GetEntitiesByClass("CustomAmmoPickup");
		if ents then
			for i,v in pairs(ents) do
				if not v.effApplied then
					UpdateForEntity(v);
				end
			end
		end
		_LASTREALCHECK=_time;
	end 
	
	if OnUpdateEx then OnUpdateEx() end
end

if OnGameRulesLoadCallback then
	OnGameRulesLoadCallback()
end
