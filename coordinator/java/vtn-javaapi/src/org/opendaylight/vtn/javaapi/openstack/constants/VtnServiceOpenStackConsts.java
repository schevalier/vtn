/*
 * Copyright (c) 2013-2014 NEC Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package org.opendaylight.vtn.javaapi.openstack.constants;

/**
 * VTN Service OpenStack related constants
 */
public class VtnServiceOpenStackConsts {

	/* Common constants */
	public static final String VTN_WEB_API_ROOT = "vtn-webapi";
	public static final String OS_RESOURCE_PKG = "org.opendaylight.vtn.javaapi.resources.openstack.";
	public static final String NOT_FOUND_SQL_STATE = "23503";
	public static final String CONFLICT_SQL_STATE = "23505";
	public static final String OS_MAC_ADD_REGEX = "^([0-9a-fA-F]{2}(\\:[0-9a-fA-F]{2}){5})$";
	public static final String OS_DATAPATH_ID_REGEX = "0[xX][0-9a-fA-F]{1,16}";
	public static final String INVALID_DATA_PATH_ID = "0XFFFFFFFFFFFFFFFF";
	public static final String VLANMAP_MODE = "vlanmap_mode";
	public static final String NULL = "null";
	public static final char X_PASS = 'p';
	public static final char X_DROP = 'd';

	/* Resource Path OpenStack URI constants */
	public static final String TENANT_PATH = "/tenants/{tenant_id}";
	public static final String TENANTS_PATH = "/tenants";
	public static final String NETWORK_PATH = "/tenants/{tenant_id}/networks/{net_id}";
	public static final String NETWORKS_PATH = "/tenants/{tenant_id}/networks";
	public static final String PORT_PATH = "/tenants/{tenant_id}/networks/{net_id}/ports/{port_id}";
	public static final String PORTS_PATH = "/tenants/{tenant_id}/networks/{net_id}/ports";
	public static final String ROUTER_PATH = "/tenants/{tenant_id}/routers/{router_id}";
	public static final String ROUTERS_PATH = "/tenants/{tenant_id}/routers";
	public static final String ROUTER_INTERFACE_PATH = "/tenants/{tenant_id}/routers/{router_id}/interfaces/{if_id}";
	public static final String ROUTER_INTERFACES_PATH = "/tenants/{tenant_id}/routers/{router_id}/interfaces";
	public static final String ROUTE_PATH = "/tenants/{tenant_id}/routers/{router_id}/routes/{route_id}";
	public static final String ROUTES_PATH = "/tenants/{tenant_id}/routers/{router_id}/routes";
	public static final String FILTER_PATH = "/filters/{filter_id}";
	public static final String FILTERS_PATH = "/filters";
	public static final String DEST_CTRL_PATH = "/destination_controller";

	/* Resource Path UNC URI constants */
	public static final String URI_CONCATENATOR = "/";
	public static final String VTN_PATH = "/vtns";
	public static final String VBRIDGE_PATH = "/vbridges";
	public static final String VLANMAP_PATH = "/vlanmaps";
	public static final String PORTMAP_PATH = "/portmap";
	public static final String VROUTER_PATH = "/vrouters";
	public static final String INTERFACE_PATH = "/interfaces";
	public static final String VLINK_PATH = "/vlinks";
	public static final String STATIC_ROUTE_PATH = "/static_iproutes";
	public static final String CTRL_PATH = "/controllers";
	public static final String SWITCH_PATH = "/switches";
	public static final String PHY_PORTS_PATH = "/ports";

	/* URI Parameter constants */
	public static final String TENANT_ID = "tenant_id";
	public static final String NET_ID = "net_id";
	public static final String PORT_ID = "port_id";
	public static final String ROUTER_ID = "router_id";
	public static final String IF_ID = "if_id";
	public static final String ROUTE_ID = "route_id";
	public static final String FILTER_ID = "filter_id";

	/* Request Parameter constants */
	public static final String ID = "id";
	public static final String DESCRIPTION = "description";
	public static final String DATAPATH_ID = "datapath_id";
	public static final String PORT = "port";
	public static final String VID = "vid";
	public static final String FILTERS = "filters";
	public static final String ROUTER_NET_ID = "net_id";
	public static final String IP_ADDRESS = "ip_address";
	public static final String DESTNATION = "destination";
	public static final String NEXTHOP = "nexthop";
	public static final String ROUTES = "routes";
	public static final String MAC_ADDRESS = "mac_address";

	/* OpenStack name's prefix constants */
	public static final String VTN_PREFIX = "os_vtn_";
	public static final String VBR_PREFIX = "os_vbr_";
	public static final String VRT_PREFIX = "os_vrt";
	public static final String IF_PREFIX = "os_if_";
	public static final String VLK_PREFIX = "os_vlk_";
	public static final String FL_PREFIX = "os_f";

	/* Database properties key name constants */
	public static final String INI_FILE_PATH = "ini_filepath";
	public static final String UNC_DB_DSN = "UNC_DB_DSN";
	public static final String DB_DRIVER = "org.postgresql.Driver";
	public static final String DB_URL_PREFIX = "jdbc:postgresql://";
	public static final String DB_IP = "Servername";
	public static final String DB_PORT = "Port";
	public static final String DB_NAME = "Database";
	public static final String DB_USER = "UserName";
	public static final String DB_PASSWORD = "Password";
	public static final String DB_INIT_CONN_SIZE = "db_initial_con_size";
	public static final String DB_MAX_CONN_SIZE = "db_max_conn_size";
	public static final String DB_WAIT_CONDITION = "db_wait_status";

	/* Resource ID key constants */
	public static final String DEFAULT_VTN = "default_vtn";
	public static final String TENANT_RES_ID = "vtn";
	public static final String NETWORK_RES_ID = "vbr";
	public static final String PORT_RES_ID = "port";
	public static final String ROUTER_RES_ID = "vrt";
	//public static final String INTERFACE_RES_ID = "if";
	public static final String DEFAULT_ROUTE = "default_route";
	public static final int MAX_ROUTER_IF_LIMIT = 393216;
	public static final String DEFAULT_IP = "0.0.0.0";
	public static final String DEFAULT_CIDR_IP = "0.0.0.0/0";
	public static final String DEFAULT_MAC = "00:00:00:00:00:00";
	public static final int MAX_MSG_LEN = 1024;
	public static final String RT_IF_FLAG = "R";
	public static final String NW_IF_FLAG = "N";
}
