--
-- Wireshark dissector for networking lab at University of Toulouse
-- Reliable Data Transfer (RDT) protocol v2.4
-- Université Toulouse III - Paul Sabatier / FSI
-- E. Lavinal
--

rdtProto = Proto("RDT-UT3", "Reliable Data Transfer (RDT) protocol v2.4")
set_plugin_info({
	version = "0.1.0",
	author = "Emmanuel Lavinal",
	description = "Dissector for Reliable Data Transfer (RDT) protocol lab"
})

local pktTypeNames = {
	[0] = "[Reserved]",
	[1] = "DATA",
	[2] = "ACK",
	[3] = "NACK",
	[4] = "CON_REQ",
	[4] = "CON_ACCEPT",
	[4] = "CON_REFUSE",
	[4] = "CON_CLOSE",
	[4] = "CON_CLOSE_ACK",
	[9] = "OTHER"
}

pktType = ProtoField.uint8("rdt.packet_type", "Packet type", base.DEC, pktTypeNames)
seqNum = ProtoField.uint8("rdt.seq_num", "Sequence number", base.DEC)
infoLen = ProtoField.uint8("rdt.info_len", "Information length", base.DEC)
checksum = ProtoField.uint8("rdt.checksum", "Checksum", base.HEX)
payload = ProtoField.string("rdt.payload", "Payload")

rdtProto.fields = {pktType, seqNum, infoLen, checksum, payload}

-- buffer: the packet
-- pinfo: the columns of the Packet List pane
-- tree:  the tree root of the Packet Details pane 
function rdtProto.dissector(buffer, pinfo, tree)

	if buffer:len() == 0 then return end

  	pinfo.cols.protocol = rdtProto.name
  	local pType = buffer(0, 1):uint()
  	local pNum = buffer(1, 1):uint()
  	if pktTypeNames[pType] == "DATA" then
	  	pinfo.cols.info = "---- DATA " .. pNum .. " --->"
	elseif pktTypeNames[pType] == "ACK" then
	  	pinfo.cols.info = "<--- ACK  " .. pNum .. " ----"
	else
		pinfo.cols.info = pktTypeNames[pType] .. " -- " .. pNum
	end

  	local subtree = tree:add(rdtProto, buffer(), "Reliable Data Transfer (RDT) protocol")

  	-- add_le: little-endian
  	-- buffer(offset, length)
  	subtree:add_le(pktType,  buffer(0, 1))
  	subtree:add_le(seqNum,   buffer(1, 1))
  	subtree:add_le(infoLen,  buffer(2, 1))
  	subtree:add_le(checksum, buffer(3, 1))
  	local payloadLen = buffer(2, 1):uint()
	subtree:add(payload, buffer(4, payloadLen))  	
end


local udpPort = DissectorTable.get("udp.port")
udpPort:add(42526, rdtProto)

