/*
 * Copyright (c) 2012-2014 NEC Corporation
 * All rights reserved.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package org.opendaylight.vtn.javaapi.ipc.enums;

/**
 * The Enum UncStructEnum.
 */
public enum UncStructEnum {

	/* No value structure */
	NONE("none"),
	/* Key and Value structures required for IPC communication */
	KeyVtn("key_vtn"),
	ValVtn("val_vtn"),
	ValVtnSt("val_vtn_st"),
	KeyVrt("key_vrt"),
	ValVrt("val_vrt"),
	ValVrtDhcpRelaySt("val_vrt_dhcp_relay_st"),
	KeyFlowList("key_flowlist"),
	ValFlowList("val_flowlist"),
	KeyDhcpRelayIf("key_dhcp_relay_if"),
	KeyDhcpRelayServer("key_dhcp_relay_server"),
	KeyVunknown("key_vunknown"),
	// vUnknown(""),
	KeyFlowListEntry("key_flowlist_entry"),
	ValFlowListEntry("val_flowlist_entry"),
	KeyVtnstationController("key_vtnstation_controller"),
	ValVtnstationControllerSt("val_vtnstation_controller_st"),
	KeyVtunnel("key_vtunnel"),
	ValVtunnel("val_vtunnel"),
	ValVtunnelSt("val_vtunnel_st"),
	KeyVtunnelIf("key_vtunnel_if"),
	ValVtunnelIf("val_vtunnel_if"),
	KeyVlink("key_vlink"),
	ValVlink("val_vlink"),
	ValVlinkSt("val_vlink_st"),
	KeyVbr("key_vbr"),
	ValVbr("val_vbr"),
	KeyVlanMap("key_vlan_map"),
	ValVlanMap("val_vlan_map"),
	ValVbrMacEntrySt(""),
	KeyVtep("key_vtep"),
	ValVtep("val_vtep"),
	ValVtepSt("val_vtep_st"),
	KeyVtepIf("key_vtep_if"),
	ValVtepIf("val_vtep_if"),
	KeyVtnFlowFilter("key_vtn_flowfilter"),
	KeyVtnFlowFilterEntry("key_vtn_flowfilter_entry"),
	ValVtnFlowFilterEntry("val_vtn_flowfilter_entry"),
	ValVunknown("val_vunknown"),
	KeyVunkIf("key_vunk_if"),
	ValVunkIf("val_vunk_if"),
	KeyVrtIf("key_vrt_if"),
	KeyVbrIfFlowFilterEntry("key_vbr_if_flowfilter_entry"),
	ValFlowfilterEntry("val_flowfilter_entry"),
	KeyVrtIfFlowFilter("key_vrt_if_flowfilter"),
	KeyVrtIfFlowFilterEntry("key_vrt_if_flowfilter_entry"),
	ValVrtArpEntrySt("val_vrt_arp_entry_st"),
	KeyVbrIfFlowFilter("key_vbr_if_flowfilter"),
	// ValVbrL2DomainSt(""),
	KeyStaticIpRoute("key_static_ip_route"),
	ValStaticIpRoute("val_static_ip_route"),
	KeyVbrFlowFilter("key_vbr_flowfilter"),
	KeyVbrFlowFilterEntry("key_vbr_flowfilter_entry"),
	ValVbrFlowFilterEntry("val_flowfilter_entry"),
	KeyVbrIf("key_vbr_if"),
	ValVbrIf("val_vbr_if"),
	ValVbrIfFlowFilter(""),
	ValVrtIf("val_vrt_if"),
	KeyVtnFlowfilterController("key_vtn_flowfilter_controller"),
	ValFlowFilterController("val_flowfilter_controller"),
	KeyVtepGrp("key_vtep_grp"),
	KeyVtepGrpMember("key_vtep_grp_member"),
	ValVtepGrp("val_vtep_grp"),
	ValPortMap("val_port_map"),

	UsessIpcSessId("usess_ipc_sess_id"),
	UsessIpcReqSessDel("usess_ipc_req_sess_del"),
	UsessIpcReqSessAdd("usess_ipc_req_sess_add"),
	UsessIpcReqSessDetail("usess_ipc_req_sess_detail"),
	KeyCtrDomain("key_ctr_domain"),
	KeySwitch("key_switch"),
	KeyCtr("key_ctr"),
	ValCtr("val_ctr"),
	ValCtrDomain("val_ctr_domain"),
	KeyBoundary("key_boundary"),
	ValBoundary("val_boundary"),

	// usess_ipc_req_sess_id(""),
	usessIpcReqUserPasswd("usess_ipc_req_user_passwd"),
	KeyPort("key_port"),
	KeyLink("key_link"),
	ValPort("val_port"),
	ValPortSt("val_port_st"),
	ValPortStNeighbour("val_port_st_neighbor"),
	KeyLogicalPort("key_logical_port"),
	KeyLogicalMemberPort("key_logical_member_port"),
	UsessIpcReqSessEnable("usess_ipc_req_sess_enable"),

	// Vtn Data Flow
	KeyVtnDataflow("key_vtn_dataflow"),
	ValVtnDataflow("val_vtn_dataflow"),
	// VTerminal
	KeyVterm("key_vterm"),
	ValVterm("val_vterm"),
	KeyVtermIf("key_vterm_if"),
	ValVtermIf("val_vterm_if"),
	ValVtermIfSt("val_vterm_if_st"),
	KeyVtermIfFlowFilter("key_vterm_if_flowfilter"),
	KeyVtermIfFlowFilterEntry("key_vterm_if_flowfilter_entry"),
	KeyDataFlow("key_dataflow"),
	KeyCtrDataFlow("key_ctr_dataflow"),
	// VTN Mapping
	KeyVtnController("key_vtn_controller"),
	ValVtnMappingControllerSt("val_vtn_mapping_controller_st");
	private String value;

	private UncStructEnum(final String value) {
		this.value = value;
	}

	public String getValue() {
		return value;
	}
}
