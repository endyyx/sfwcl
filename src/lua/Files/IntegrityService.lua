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