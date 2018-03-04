IntegrityService = {
	type = "IntegrityService",
	Properties = {
	
	},
	Editor={
		
	},
	Server = {},
	Client = {}
};
function IntegrityService.Server:SvOnReceiveMessage(id, msgType, msg)
	-- Server implementation
end
function IntegrityService.Server:SvCheckSignature(id, signature, a1, a2, length)
	-- Server implementation
end
function IntegrityService.Client:ClOnReceiveMessage(id,msgType,msg)
	OnServerMessage(self, id, msgType, msg)
end
function IntegrityService.Client:ClRequestSignature(id, nonce, a1, a2, length)
	self.server:SvCheckSignature(id, CPPAPI.SignMemory(a1, a2, length, nonce, id), a1, a2, length)
end
Net.Expose {
	Class = IntegrityService,
	ClientMethods = {
		ClOnReceiveMessage					= { RELIABLE_UNORDERED, POST_ATTACH, STRING, STRING, STRING },
		ClRequestSignature					= { RELIABLE_UNORDERED, POST_ATTACH, STRING, STRING, STRING, STRING, STRING},
	},
	
	ServerMethods = {
		SvOnReceiveMessage		 			= { RELIABLE_UNORDERED, POST_ATTACH, STRING, STRING, STRING },
		SvCheckSignature					= { RELIABLE_UNORDERED, POST_ATTACH, STRING, STRING, STRING, STRING, STRING},
	},
	ServerProperties = {
		--...
	}
};
function SendMessage(msg)
	local ent = System.GetEntityByName("IntegrityServiceEntity")
	if ent then ent.server:SvOnReceiveMessage(tostring(msg)) end
end
function RequestSignature(channelId, id, nonce, addr1, addr2, length)
	local ent = System.GetEntityByName("IntegrityServiceEntity")
	if ent then ent.onClient:ClRequestSignature(channelId, id, nonce, addr1, addr2, tostring(length)) end
end
function SendMessageToClient(channelId, msg)
	local ent = System.GetEntityByName("IntegrityServiceEntity")
	if ent then ent.onClient:ClOnReceiveMessage(channelId, msg) end
end
System.LogAlways("IntegrityService loaded");