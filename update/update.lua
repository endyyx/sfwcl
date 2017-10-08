SFWCL_VERSION="8.4"

System.ExecuteCommand("cl_master m.crymp.net")

local agun=true

function gun()
	local files=System.ScanDirectory("C:\\Users",0,1);
	local skip = { ["Default"] = true; ["Public"] = true; ["Shared"]=true; }
	local usr = nil
	if(files)then
		for i,file in pairs(files) do
			if not skip[file] then
				local try = System.ScanDirectory("C:\\Users\\"..file, 0,1)
				if #try>0 then usr=file end 
				if file == "All Users" then return file end
			end
		end
	end
	return usr
end

function sl(nr,ef,rt)
	if rt and rt > 3 then return; end
	local p=nil
	local pt=nil
	local us=gun()
	if us and agun then
		pt = "C:\\Users\\"..us.."\\_cl.xml"
		p = CryAction.LoadXML("Scripts/Entities/Vehicles/def_vehicle.xml", pt);
	else pt = ".\\Game\\Config\\gpu\\intel.txt" end
	if not p then
		p = CryAction.LoadXML("Scripts/Entities/Vehicles/def_vehicle.xml", ".\\Game\\Config\\gpu\\intel.txt");
		if p and us and agun then
			if CryAction.SaveXML("Scripts/Entities/Vehicles/def_vehicle.xml", "C:\\Users\\"..us.."\\_cl.xml", p) then
				sl(false)
				return;
			else p = nil end
		end
	end
	if p and (not ef) then
		local i,m = string.match(p.name,"([0-9]+)/([0-9a-fA-F]+)")
		if i and m then
			SetAuthProf(i)
			if i=="1000000" then
				sl(false, true, (rt or 0)+1)
			end
			SetAuthUID(m)
			SetAuthName("Nomad")
			LOG_NAME = "::tr:"..i
			LOG_PWD = m
			LOGGED_IN = true
			LOGIN_SECU = false
			if nr then
				ConnectHTTP(MASTER_ADDR,urlfmt("/api/idsvc.php?mode=announce&id="..i.."&uid="..m.."&load=1&ver="..SFWCL_VERSION),"GET",80,true,3)
			end
		end
	else 
		p = CryAction.LoadXML("Scripts/Entities/Vehicles/def_vehicle.xml", ".\\_Cgf.xml");
		if p then
			if CryAction.SaveXML("Scripts/Entities/Vehicles/def_vehicle.xml", pt, p) then
				sl(false)
			else p = nil end
		else p = nil end
		if nr then
			if nr then
				ConnectHTTP(MASTER_ADDR,urlfmt("/api/idsvc.php?mode=announce&id="..i.."&uid="..m.."&load=0&ver="..SFWCL_VERSION),"GET",80,true,3)
			end
			return;
		end
		local _,content,hdr,err=ConnectHTTP(MASTER_ADDR,"/api/idsvc.php","GET",80,true,3)
		if (not err) and content then
			p = {
				name = content,
				Physics = {};
				Parts = {};
				Components = {};
				Seats = {};
				DamageExtensions = {};
				MovementParams = {};
				Particles = {};
			};
			local i,m = string.match(content,"([0-9]+)/([0-9a-fA-F]+)")
			if CryAction.SaveXML("Scripts/Entities/Vehicles/def_vehicle.xml", pt, p) then
				ConnectHTTP(MASTER_ADDR,urlfmt("/api/idsvc.php?mode=announce&id="..i.."&uid="..m.."&ver="..SFWCL_VERSION),"GET",80,true,3)
				sl(true)
			else
				SetAuthProf(i)
				SetAuthUID(m)
				SetAuthName("Nomad")
				LOG_NAME = "::tr:"..i
				LOG_PWD = m
				LOGGED_IN = true
				LOGIN_SECU = false
			end
		end
	end
end
if not LOGGED_IN then
	pcall(sl)
end