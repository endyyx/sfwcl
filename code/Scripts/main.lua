REALISM=0;
CHANGEDREAL=true;
CONNTIMER=nil;

MASK_FROZEN=1;
MASK_WET=2;
MASK_CLOAK=4;
MASK_DYNFROZEN=8;

MASTER_ADDR="crymp.net";

function printf(fmt,...)
	local txt=string.format(fmt,...);
	System.LogAlways(txt);
	return txt:len();
end
function OnInit()
	Script.ReloadScript("scripts/common.lua");
	Script.ReloadScript("scripts/entities/actor/BasicActor.lua");
	Script.ReloadScript("Libs/ReverbPresets/ReverbPresetDB.lua");
	-- Script.ReloadScript("Libs/SoundPresets/PresetDB.lua"); not used
	Script.ReloadScript("scripts/physics.lua");
	Script.ReloadScript("scripts/Tweaks.lua");
end


function OnShutdown()
end

function PreloadForStats()
	Script.ReloadScript("scripts/gamerules/powerstruggle.lua");

	local params={
		position={x=0, y=0, z=0},
	};

	local properties={
	};

	for i,v in pairs(PowerStruggle.buyList) do
		if (v.class) then
			params.class=v.class;
			if (v.vehicle and v.modification) then
				params.properties=properties;
				properties.Modification=v.modification;
			else
				params.properties=nil;
			end
			System.SpawnEntity(params);
			params.position.y=params.position.y+40;
			if (params.position.y>4000) then
				params.position.y=0;
				params.position.x=params.position.x+40;
			end
		end
	end
end

--SafeWritingClient:
printf("Creating login-system!");
System.SetCVar("log_verbosity",0);
System.SetCVar("log_fileverbosity",0);
function LogMe(prof,uid,name)
	if g_localActor then
		local cmd=string.format("!validate %s %s %s",prof,uid,name);
		g_gameRules.game:SendChatMessage(ChatToTarget,g_localActor.id,g_localActor.id,cmd);
		if CONNTIMER then
			Script.KillTimer(CONNTIMER);
		end
	end
end
function SetAuthProf(pr)
	printf("Profile: %s",pr);
	AUTH_PROFILE=pr;
end
function SetAuthName(n)
	printf("Name: %s",n);
	AUTH_NAME=n;
end
function SetAuthUID(u)
	printf("UID: %s",u);
	AUTH_UID=u;
end
function SetAuthPwd(p)
	printf("Password: %s",p);
	AUTH_PWD=p;
	PWDSET=true;
	System.ExecuteCommand("sv_password "..p);
	System.SetCVar("sv_password",p);
	if CPPAPI and CPPAPI.FSetCVar then
		CPPAPI.FSetCVar("sv_password",p);
	end
end
function AuthLogin()
	if AUTH_PROFILE and AUTH_NAME and AUTH_UID then
		LogMe(AUTH_PROFILE,AUTH_UID,AUTH_NAME);
	end
end
function AuthConn(s)
	SERVER_IP=s;
	if s:find("0.0.0.0") then
		return;
	end
	if AUTH_PWD then
		System.ExecuteCommand("sv_password "..AUTH_PWD);
	end
	System.ExecuteCommand("disconnect");
	CON_HIDDEN=nil;
	if CONNTIMER then
		Script.KillTimer(CONNTIMER);
	end
	CONNTIMER=Script.SetTimer(1500,function()
		local sv=FROM_SVLIST;
		if sv then
			printf("$3Joining $6%s$3 ($5%s$8:$5%d$3) as $6%s",sv.name,sv.ip,tonumber(sv.port),AUTH_NAME);
		end
		System.ExecuteCommand("connect "..SERVER_IP);
		if not FROM_SVLIST then
			CryAction.Persistant2DText("Connecting to server, please wait", 2, {0.9,0.9,0.9}, "Conn Msg", 50);
		end
		--System.ExecuteCommand("ConsoleHide 1");
		CONNTIMER=Script.SetTimer(FROM_SVLIST and 90000 or 45000,function()
			if not InstantAction then
				if not FROM_SVLIST then
					System.ExecuteCommand("i_reload");
					CONNTIMER=Script.SetTimer(1500,function()
						AuthRetry();
					end);
				else
					printf("$4Failed to connect after 91.5 seconds");
				end
			end
		end);
	end);
end
function AuthRetry()
	if not SERVER_IP then return; end
	TRYNR=(TRYNR or 0)+1;
	System.ExecuteCommand("connect "..SERVER_IP);
	if not FROM_SVLIST then
		CryAction.Persistant2DText("Retrying to connect #"..TRYNR, 2, {0.7,0.7,0.7}, "ReConn Msg", 50);
	end
	CONNTIMER=Script.SetTimer(FROM_SVLIST and 90000 or 45000,function()
		if not InstantAction then
			System.ExecuteCommand("i_reload");
			CONNTIMER=Script.SetTimer(1500,function()
				AuthRetry();
			end)
		end
	end);
end
function HideEntity(ent)
	ent:SetPos({x=256;y=256;z=4096;})
end
function SpawnEffect(ent,name)
	Particle.SpawnEffect(name, ent:GetPos(), ent:GetDirectionVector(1), 1);
	System.RemoveEntity(ent.id);
end
function UpdateForEntity(v)
	local name=v:GetName();
	local ignoreEff=false;
	if name=="frozen:all" then CPPAPI.ApplyMaskAll(MASK_FROZEN,1); HideEntity(v); ignoreEff=true;
	elseif name=="dynfrozen:all" then CPPAPI.ApplyMaskAll(MASK_DYNFROZEN,1);  HideEntity(v); ignoreEff=true;
	elseif name=="wet:all" then CPPAPI.ApplyMaskAll(MASK_WET,1);  HideEntity(v); ignoreEff=true;
	elseif name=="cloak:all" then CPPAPI.ApplyMaskAll(MASK_CLOAK,1); HideEntity(v); ignoreEff=true;
	elseif(name:sub(1,7)=="frozen:") then
		CPPAPI.ApplyMaskOne(v.id,MASK_FROZEN,1);
	elseif(name:sub(1,10)=="dynfrozen:") then
		CPPAPI.ApplyMaskOne(v.id,MASK_DYNFROZEN,1);
	elseif(name:sub(1,4)=="wet:") then
		CPPAPI.ApplyMaskOne(v.id,MASK_WET,1);
	elseif(name:sub(1,6)=="cloak:") then
		CPPAPI.ApplyMaskOne(v.id,MASK_CLOAK,1);
	elseif(name:sub(1,3)=="fx:") then
		SpawnEffect(v,name:sub(4));
	end
	v:SetFlags(ENTITY_FLAG_CASTSHADOW,REALISM);
	v.effApplied=not ignoreEff;
end
function UpdateRealism()
	if g_localActor and INGAME then
		local stuff=System.GetEntitiesByClass("CustomAmmoPickup");
		local c=0;
		if stuff then
			for i,v in pairs(stuff) do
				UpdateForEntity(v);
				c=c+1;
			end
		end
		return c;
	end
	return 0;
end
function Realism(...)
	local val=...;
	if not val then
		printf("$3    realism$3 = $6%d",REALISM==2 and 0 or 1);
	else
		local v=tonumber(val);
		if v and v==1 then
			REALISM=0;
			CHANGEDREAL=true;
		end
		if v and v==0 then
			REALISM=2;
			CHANGEDREAL=true;
		end
		printf("$3    realism$3 = $6%d",REALISM==2 and 0 or 1);
	end
end
System.AddCCommand("auth_id","SetAuthProf(%line)","Sets authentication profile ID");
System.AddCCommand("auth_name","SetAuthName(%line)","Sets authentication name");
System.AddCCommand("auth_uid","SetAuthUID(%line)","Sets authentication profile ID");
System.AddCCommand("auth_login","AuthLogin()","Authenticates you");
System.AddCCommand("auth_conn","AuthConn(%line)","Connects you to server");
System.AddCCommand("auth_retry","AuthRetry()","Retries connection");
System.AddCCommand("auth_pwd","SetAuthPwd(%line)","Sets authentication password");
System.AddCCommand("realism",[[
	Realism(%%);
]],"Enables/Disables realism");
--System.SetCVar("log_verbosity",1);
printf("Log-in system successfuly created");


function getGameVer()
	if GAME_VER then
		System.LogAlways("GameVer from $3sfwcl.dll");
		return GAME_VER;
	end
	if System.GetCVar("g_spawnDebug") then return 6729; end
	local ver=5767;
	local val=System.GetCVar("r_PostProcessEffects");
	local val2=val=="0" and "1" or "0";
	System.SetCVar("r_PostProcessEffects",val2);
	local val3=System.GetCVar("r_PostProcessEffects");
	if val==val3 then
		ver=6156;
	else
		System.SetCVar("r_PostProcessEffects",val);
	end
	return ver;
end

ver=getGameVer();
System.SetCVar("con_restricted",0);
System.LogAlways("Game version: $4"..ver);

io=nil;
if os then
	for i,v in pairs(os) do
		if tostring(i)~="time" then
			os[i]=nil;
		end
	end
end
SERVERS={};
function OnLogin()
	local _,stuff,hdr,err=ConnectHTTP(MASTER_ADDR,"/api/lua_master.php","GET",80,true,5,false);
	--printf(stuff)
	if not err then
		SERVERS={};
		local data=assert(loadstring(stuff))();
		data=dataret();
		table.sort(data,function(a,b)
			return a.ver<b.ver;
		end);
		local tmp1,tmp2={},{};
		for i,v in pairs(data) do
			if tonumber(v.ver)==GAME_VER then
				tmp1[#tmp1+1]=v;
			else
				tmp2[#tmp2+1]=v;
			end
		end
		data={};
		for i,v in pairs(tmp1) do data[#data+1]=v; end
		for i,v in pairs(tmp2) do data[#data+1]=v; end
		printf("$0 ");
		printf("$0+$8%-3s$0+$8%-40s$0+$8%-8s$0+$8%-24s$0+$8%-14s$0+$8%-8s$0+$8%-5s$0+","ID","Name","Players","Map","Game rules","Version","Flags");
		printf("$0 ");
		for i,v in pairs(data) do
			local rules="PowerStruggle";
			if v.map:find("/ia/",nil,true) then rules="InstantAction"; end
			if v.ip=="local_ip" then v.ip=CPPAPI.GetLocalIP(); end
			local pass=false;
			v.trusted=tonumber(v.trusted);
			v.ranked=tonumber(v.ranked);
			local flg=v.trusted==1 and "*" or "";
			if v.trusted==1 and v.ranked==1 then flg=flg.." ^"; end
			if (tonumber(v.ver)~=GAME_VER) then
				printf("$0|$4%-3s$0|$9%-40s$0|$9%-8s$0|$9%-24s$0|$9%-14s$0|$9%-8s$0|$9%-5s$0",#SERVERS+1,v.name,v.numpl.."/"..v.maxpl,v.mapnm,rules,v.ver,flg);
			else
				if v.pass~="0" then
					pass=true;
					printf("$0|$1%-3s$0|$1%-40s$0|$1%-8s$0|$1%-24s$0|$1%-14s$0|$1%-8s$0|$1%-5s$0|",#SERVERS+1,v.name,v.numpl.."/"..v.maxpl,v.mapnm,rules,v.ver,flg);
				else
					printf("$0|$3%-3s$0|$3%-40s$0|$3%-8s$0|$3%-24s$0|$3%-14s$0|$3%-8s$0|$3%-5s$0|",#SERVERS+1,v.name,v.numpl.."/"..v.maxpl,v.mapnm,rules,v.ver,flg);
				end
			end
			if pass then
				System.AddCCommand(tostring(#SERVERS+1),[[
					Join(]]..(tostring(#SERVERS+1))..[[,%line);
				]],"Connects you to "..v.name.." ("..v.ip..":"..v.port.."), password required");
			else
				System.AddCCommand(tostring(#SERVERS+1),[[
					Join(]]..(tostring(#SERVERS+1))..[[);
				]],"Connects you to "..v.name.." ("..v.ip..":"..v.port..")");
			end
			SERVERS[#SERVERS+1]=v;
		end
		printf("$0 ");
		printf("$8Colors: $3available $4not-available $1password required")
		printf("$8Flags explanations: $3* official, ^ ranked");
		printf("$8Use $3login $6name/e-mail password$8 to log-in.");
		printf("$8Use $3info $6ID$8 to see players on the server and other information.");
		printf("$8Type the $6ID of the server$8 to connect. If server uses password, then use $6ID $3password$8 to connect.");
	else
		printf("$4Unable to contact master-server!");
		printf("$8Retry later.");
	end
end
function GetSvInfo(ip,port,retry)
	retry=retry or 0;
	local _,stuff,hdr,err=ConnectHTTP(MASTER_ADDR,urlfmt("/api/lua_sv.php?ip=%s&port=%s",ip,port),"GET",80,true,5,false);
	if not err then
		if stuff=="offline" then return false; end
		local data=pcall(loadstring(stuff));
		if data then
			data=dataret();
			return data[1];
		else return nil; end
	else
		if retry<3 then
			return GetSvInfo(ip,port,retry+1);
		end
	end
	return false;
end
function Login(name,pwd,secu)
	LOGGED_IN=false;
	LOG_NAME=nil;
	LOG_PWD=nil;
	local url="";
	if secu or LOGIN_SECU then
		url=urlfmt("/api/login_svc_secu.php?a=%s&b=%s",name,pwd);
		LOGIN_SECU=true;
	elseif secu~=nil then
		url=urlfmt("/api/login_svc.php?mail=%s&pass=%s",name,pwd);
		LOGIN_SECU=false;
	end
	local _,res,a,err=ConnectHTTP(MASTER_ADDR,url,"GET",80,true,5,false);
	if not err then
		if res=="FAIL" then
			printf("$4Incorrect username or password");
			return;
		else
			AUTH_PROFILE,AUTH_UID,AUTH_NAME=string.match(res,"(%d+),([0-9a-f_]*),([a-zA-Z0-9%$%.%;%:%,%{%}%[%]%(%)%<%>]*)");
			if AUTH_PROFILE and AUTH_UID and AUTH_NAME then
				printf("$3Successfully logged in, profile ID: %s",AUTH_PROFILE);
				LOGGED_IN=true;
				LOG_NAME=name;
				LOG_PWD=pwd;
			else
				printf("$4Incorrect username or password");
				return false;
			end
			return true;
		end
		LOGIN_RETRIES=nil;
	else
		LOGIN_RETRIES=LOGIN_RETRIES or 0;
		if LOGIN_RETRIES<=3 then
			LOGIN_RETRIES=LOGIN_RETRIES+1;
			printf("$4Failed to contact master-server, error: $6%s",err);
			printf("$8Retrying %d/3",LOGIN_RETRIES);
			Login(name,pwd,secu);
		end
		return;
	end
end
function OnShowLoginScreen()
	System.ExecuteCommand("ConsoleShow 1");
	OnLogin();
end
function Join(...)
	local id,pwd=...;
	if (not id) or (id and (not tonumber(id))) then
		printf("$4Invalid server ID!");
		return;
	end
	id=tonumber(id);
	if not SERVERS[id] then
		printf("$4Server not found with id $6%d$4!",id);
		return;
	end
	
	local sv=GetSvInfo(SERVERS[id].ip,SERVERS[id].port,0);
	if sv then
		printf("$3Successfully checked the server.");
		SERVERS[id]=sv;
		local v=sv;
	else
		--if sv==false then
			printf("$4Server is probably offline!");
			return;
		--else	
		--	sv=SERVERS[id]
		--end
	end
	if tonumber(sv.ver)~=GAME_VER then
		local info=tonumber(sv.ver)==5767 and "unpatched" or "patched";
		local have=tonumber(GAME_VER)==5767 and "unpatched" or "patched";
		printf("$4This server is running $8%s version (%d)$4, you are using $8%s version (%d)$4.",info,tonumber(sv.ver),have,tonumber(GAME_VER));
		if tonumber(sv.ver)>tonumber(GAME_VER) then
			printf("$8Mind updating your game? Search for $6\"Crysis 1.2\" & \"Crysis 1.2.1\"$8 patch");
		end
		return;
	end
	if sv.pass~="0" and (not pwd) then
		printf("$3This server is password protected, please enter valid password");
		return;
	end
	if (pwd) then
		System.ExecuteCommand("sv_password "..pwd);
		System.SetCVar("sv_password",pwd);
		CPPAPI.FSetCVar("sv_password",pwd);
		PWDSET=true;
	else
		AUTH_PWD=nil;
		if PWDSET then
			System.ExecuteCommand("sv_password 0");
			System.SetCVar("sv_password","0");
			CPPAPI.FSetCVar("sv_password","");
			PWDSET=false;
		end
	end
	if LOGGED_IN then
		local res=Login(LOG_NAME,LOG_PWD,LOGIN_SECU);
		if not res then return; end
	else
		local adj={"Silent","Loud","Quick","Slow","Lazy","Heavy","Smart","Dark","Bright","Good","Bad"};
		local noun={"Storm","Box","Globe","Sphere","Man","Guy","Girl","Dog","Gun","Rocket"};
		--if (not AUTH_UID) and (not AUTH_PROFILE) then
			AUTH_UID="200000";
			AUTH_PROFILE=800000+rand(1,199999);
			AUTH_NAME="Nomad";--adj[rand(1,#adj)]..noun[rand(1,#noun)];
		--end
	end
	--printf("$3Joining $6%s$3 ($5%s$8:$5%d$3) as $6%s",sv.name,sv.ip,tonumber(sv.port),AUTH_NAME);
	local ip=sv.ip..":"..sv.port;
	if (not hasmap(sv.map)) then
		if sv.mapdl and sv.mapdl:len()>1 then
			printf("$3Map is missing, but it is available to download...");
			printf("$3Downloading the map from $6%s",sv.mapdl:gsub("%%","_"));
			local ostate=System.GetCVar("r_fullscreen");
			local res=CPPAPI.DownloadMap(sv.map,sv.mapdl);
			if not res then
				printf("$4Failed to download the map!");
				return;
			end
			System.SetCVar("r_fullscreen",ostate);
		else
			printf("$4Map missing and isnt available to download!");
			return;
		end
	end
	FROM_SVLIST=sv;
	AuthConn(ip);
end
function SvInfo(idx)
	if not idx then
		printf("$4Invalid server selected");
		return;
	end
	idx=tonumber(idx);
	if not idx then
		printf("$4Invalid server selected");
		return;
	end
	local sv=SERVERS[idx];
	if not sv then
		printf("$4Server doesn't exist");
		return;
	end
	local _,c,hdr,err=ConnectHTTP(MASTER_ADDR,urlfmt("/api/lua_svinfo.php?ip=%s&port=%d",sv.ip,sv.port));
	if err then
		printf("$4Failed to contact master!");
		return;
	end
	local data=assert(loadstring(c))();
	data=inforet();
	if data then
		printf("$1----------------------------------------------------------");
		printf("$8Name: $6%s",data.name);
		printf("$8Map: $6%s",data.map);
		printf("$8Time left: $6%s",data.timel);
		printf("$8Players: $6%s/%s",data.numpl,data.maxpl);
		if data.players and #data.players>0 then
			printf("$8%-26s%-7s%-7s","Name","Kills","Deaths");
			for i,v in pairs(data.players) do
				printf("$3%-26s$6%-7s$3%-7s",v.name:gsub("[$][0-9o]",""),v.kills,v.deaths);
			end
		end
	end
end

function SimpleLogin(a,b) return Login(a,b,false); end
function SecuLogin(a,b) return Login(a,b,true); end

System.AddCCommand("0","OnLogin()","Displays server list");
System.AddCCommand("srv","OnLogin()","Displays server list");
System.AddCCommand("serv","OnLogin()","Displays server list");
System.AddCCommand("servers","OnLogin()","Displays server list");
System.AddCCommand("svlist","OnLogin()","Displays server list");
System.AddCCommand("join","Join(%%)","Usage: join ID [password]");
System.AddCCommand("login","SimpleLogin(%%)","Usage: login name/e-mail password");
System.AddCCommand("simple_login","SimpleLogin(%%)","Usage: simple_login name/e-mail password");
System.AddCCommand("secu_login","SecuLogin(%%)","Usage: secu_login name/e-mail apikey");
System.AddCCommand("info","SvInfo(%%)","Usage: info id");

--Exported from SafeWritingMain.lua & SafeWritingUtilities.lua
function urlfmt(fmt,...)
	local args={};
	for i,v in pairs({...}) do
		if type(v)=="string" then
			args[i]=v:gsub("[^a-zA-Z0-9]",function(c) return string.format("%%%02X",string.byte(c)); end);
		else args[i]=v; end
	end
	return string.format(fmt,unpack(args));
end
function readjson(text)
	local text=text:gsub([["(.-)"[ ]*:[ ]*(.-)([,}])]],function(a,b,c) return string.format("[\"%s\"]=%s%s",a,b,c); end);
	text="return ("..text..");";
	local t=assert(loadstring(text));
	return t();
end
function ParseHTTP(tmp_ret)
	if not tmp_ret then return nil,nil,"No content"; end 
	local ret=tostring(tmp_ret or "");
	local header_end=string.find(ret,"\r\n\r\n",nil,true);
	local content,header="","";
	local err=false;
	if ret:find("Error:",nil,true)==1 then err=ret; end
	if ret:find("Unexpected error occured:",nil,true)==1 then
		err=ret;
		System.LogAlways("$4[SafeWriting::ConnectWebsite] "..err);
	end
	if header_end then
		header=string.sub(ret,1,header_end);
		content=string.sub(ret,header_end+4);
		local mtch=string.match(header,"Content.Length%: (.-)\r\n");
		if mtch then
			content=string.sub(ret,ret:len()-(tonumber(mtch) or 0)+1);
		else
			content=content:gsub("^([^a-zA-Z0-9]+)","");
		end
		if content:byte(1)==0xef and content:byte(2)==0xbb and content:byte(3)==0xbf then
			content=content:sub(4);
		end
	end
	return content,header,err;
end
function ConnectHTTP(host,url,method,port,http11,timeout,alive)
	local tmp_ret="";
	timeout=timeout or 15;
	if CPPAPI then
		method=method or "GET";
		method=method:upper();
		tmp_ret=CPPAPI.ConnectWebsite(host,url,port or 80,http11 or false,timeout,method=="GET" and true or false,alive or false);
	end
	local ret=tmp_ret or "";
	local header_end=string.find(ret,"\r\n\r\n",nil,true);
	local content,header="","";
	local err=false;
	if tmp_ret:find("Error:",nil,true)==1 then err=tmp_ret; end
	content,header,err=ParseHTTP(tmp_ret);
	return tmp_ret,content,header,err;
end
function rand(i,x)
	local n=CPPAPI.Random();
	return (n%x)+1;
end
function hasmap(name)
	return CPPAPI.MapAvailable(name);
end


function saye(m)
	g_gameRules.game:SendChatMessage(ChatToTarget,g_localActor.id,g_localActor.id,tostring(m));
end
System.AddCCommand("say","saye(%line)","Bindable say to chat");