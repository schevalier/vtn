/*
 * Copyright (c) 2012-2014 NEC Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */

/*
 * dal_schema.hh - contians schema information of all tables
 *
 */
#ifndef DAL_DAL_SCHEMA_HH_
#define DAL_DAL_SCHEMA_HH_

#include <stdint.h>
#include <string.h>
#include <sqlext.h>
//#include <sqltypes.h>
// #include "dal_defines.hh"

namespace unc {
namespace upll {
namespace dal {

typedef uint16_t DalColumnIndex;
typedef uint16_t DalTableIndex;

namespace schema {
/* Type Definition of Schema structure */
struct DalColumnSchema {
  const char *column_name;       // Column Name
  SQLSMALLINT db_data_type_id;   // DB Data type Identifier from unixodbc
  SQLSMALLINT dal_data_type_id;  // DAL C Data type Identifier from unixodbc
  size_t db_array_size;          // size used in DB
};
struct DalTableSchema {
  const char *table_name;   // Table Name
  size_t num_columns;      // Total number of columns in the table
  size_t num_pk_columns;   // Total number of primary key columns
  const DalColumnSchema *column_schema;
       // pointer to array of column_schema, indexed by DalColumnIndex
};
namespace table {
// L2/L3 MoMgr Enum Start
namespace vtn {
static const uint8_t kVtnNumPks = 1;
enum kVtnIndex {
  kDbiVtnName = 0,
  kDbiVtnDesc,
  kDbiVtnOperStatus,
  kDbiVtnAlarmStatus,
  kDbiDownCount,
  kDbiVtnCreationTime,
  kDbiVtnLastUpdatedTime,
  kDbiVtnFlags,
  kDbiValidVtnDesc,
  kDbiValidVtnOperStatus,
  kDbiValidVtnAlarmStatus,
  kDbiValidVtnCreationTime,
  kDbiValidVtnLastUpdatedTime,
  kDbiVtnCsRowStatus,
  kDbiVtnCsDesc,
  kDbiVtnNumCols
};
}

namespace vtn_controller {
static const uint8_t kVtnCtrlrNumPks = 3;
enum kVtnCtrlrIndex {
  kDbiVtnName = 0,
  kDbiControllerName,
  kDbiDomainId,
  kDbiOperStatus,
  kDbiAlarmStatus,
  kDbiDownCount,
  kDbiRefCount,
  kDbiVtnCtrlrFlags,
  kDbiValidOperStatus,
  kDbiValidAlarmStatus,
  kDbiCsRowstatus,
  kDbiCsDesc,
  kDbiVtnCtrlrNumCols
};
}

namespace vtn_rename {
static const uint8_t kVtnRenameNumPks = 3;
enum kVtnRenameIndex {
  kDbiCtrlrVtnName = 0,
  kDbiControllerName,
  kDbiDomainId,
  kDbiUncVtnName,
  kDbiVtnRenameNumCols
};
}

namespace vbridge {
static const uint8_t kVbrNumPks = 2;
enum kVbrIndex {
  kDbiVtnName = 0,
  kDbiVbrName,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiVbrDesc,
  kDbiHostAddr,
  kDbiHostAddrMask,
  kDbiOperStatus,
  kDbiDownCount,
  kDbiFaultCount,
  kDbiValidCtrlrName,
  kDbiValidDomainId,
  kDbiValidVbrDesc,
  kDbiValidHostAddr,
  kDbiValidHostAddrMask,
  kDbiValidOperStatus,
  kDbiCsRowStatus,
  kDbiCsCtrlrName,
  kDbiCsDomainId,
  kDbiCsVbrDesc,
  kDbiCsHostAddr,
  kDbiCsHostAddrMask,
  kDbiVbrFlags,
  kDbiVbrNumCols
};
}

namespace vbridge_vlanmap {
static const uint8_t kVbrVlanMapNumPks = 4;
enum kVbrVlanMapIndex {
  kDbiVtnName = 0,
  kDbiVbrName,
  kDbiLogicalPortId,
  kDbiLogicalPortIdValid,
  kDbiVlanid,
  kDbiBdryRefCount,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiVbrVlanMapFlags,
  kDbiValidVlanid,
  kDbiValidBdryRefCount,
  kDbiCsRowStatus,
  kDbiCsVlanid,
  kDbiVbrVlanMapNumCols
};
}


namespace vbridge_interface {
static const uint8_t kVbrIfNumPks = 3;
enum kVbrIfIndex {
  kDbiVtnName = 0,
  kDbiVbrName,
  kDbiIfName,
  kDbiAdminStatus,
  kDbiDesc,
  kDbiLogicalPortId,
  kDbiVlanId,
  kDbiTagged,
  kDbiVexName,
  kDbiVexIfName,
  kDbiVexLinkName,
  kDbiOperStatus,
  kDbiDownCount,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiFlags,
  kDbiValidAdminStatus,
  kDbiValidDesc,
  kDbiValidPortMap,
  kDbiValidLogicalPortId,
  kDbiValidVlanid,
  kDbiValidTagged,
  kDbiValidVexName,
  kDbiValidVexIfName,
  kDbiValidVexLinkName,
  kDbiValidOperStatus,
  kDbiCsRowstatus,
  kDbiCsAdminStatus,
  kDbiCsDesc,
  kDbiCsPortMap,
  kDbiCsLogicalPortId,
  kDbiCsVlanid,
  kDbiCsTagged,
  kDbiVbrIfNumCols
};
}

namespace vrouter {
static const uint8_t kVrtNumPks = 2;
enum kVrtIndex {
  kDbiVtnName = 0,
  kDbiVrtName,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiVrtDesc,
  kDbiDhcprelayAdminstatus,
  kDbiOperStatus,
  kDbiDownCount,
  kDbiVrtFlags,
  kDbiValidCtrlrName,
  kDbiValidDomainId,
  kDbiValidDesc,
  kDbiValidDhcprelayAdminstatus,
  kDbiValidOperStatus,
  kDbiCsRowstatus,
  kDbiCsCtrlrName,
  kDbiCsDomainId,
  kDbiCsVrtDesc,
  kDbiCsDhcprelayAdminstatus,
  kDbiVrtNumCols
};
}

namespace vrouter_interface {
static const uint8_t kVrtIfNumPks = 3;
enum kVrtIfIndex {
  kDbiVtnName = 0,
  kDbiVrouterName,
  kDbiIfName,
  kDbiDesc,
  kDbiIpAddr,
  kDbiMask,
  kDbiMacAddr,
  kDbiAdminStatus,
  kDbiOperStatus,
  kDbiDownCount,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiVrtIfFlags,
  kDbiValidDesc,
  kDbiValidIpAddr,
  kDbiValidMask,
  kDbiValidMacAddr,
  kDbiValidAdminStatus,
  kDbiValidOperStatus,
  kDbiCsRowstatus,
  kDbiCsDesc,
  kDbiCsIpAddr,
  kDbiCsMask,
  kDbiCsMacAddr,
  kDbiCsAdminStatus,
  kDbiVrtIfNumCols
};
}

namespace vnode_rename {
static const uint8_t kVnodeRenameNumPks = 4;
enum kVnodeRenameIndex {
  kDbiCtrlrVtnName = 0,
  kDbiCtrlrVnodeName,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiUncVtnName,
  kDbiUncvnodeName,
  kDbiVnodeRenameNumCols
};
}

namespace vlink {
static const uint8_t kVlinkNumPks = 2;
enum kVlinkIndex {
  kDbiVtnName = 0,
  kDbiVlinkName,
  kDbiAdminStatus,
  kDbiVnode1Name,
  kDbiVnode1Ifname,
  kDbiVnode2Name,
  kDbiVnode2Ifname,
  kDbiBoundaryName,
  kDbiVlanid,
  kDbiDesc,
  kDbiOperStatus,
  kDbiDownCount,
  kDbiCtrlr1Name,
  kDbiCtrlr2Name,
  kDbiDomain1Id,
  kDbiDomain2Id,
  kDbiKeyFlags,
  kDbiValFlags,
  kDbiValidAdminStatus,
  kDbiValidVnode1Name,
  kDbiValidVnode1Ifname,
  kDbiValidVnode2Name,
  kDbiValidVnode2Ifname,
  kDbiValidBoundaryName,
  kDbiValidVlanid,
  kDbiValidDesc,
  kDbiValidOperStatus,
  kDbiCsRowstatus,
  kDbiCsAdminStatus,
  kDbiCsVnode1Name,
  kDbiCsVnode1Ifname,
  kDbiCsVnode2Name,
  kDbiCsVnode2Ifname,
  kDbiCsBoundaryName,
  kDbiCsVlanid,
  kDbiCsDesc,
  kDbiVlinkNumCols
};
}

namespace vlink_rename {
static const uint8_t kVlinkRenameNumPks = 4;
enum kVlinkRenameIndex {
  kDbiCtrlrVtnName = 0,
  kDbiCtrlrVlinkName,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiVtnName,
  kDbiVlinkName,
  kDbiVlinkRenameNumCols
};
}

namespace static_ip_route {
static const uint8_t kStaticIpRouteNumPks = 6;
enum kStaticIpRouteIndex {
  kDbiVtnName = 0,
  kDbiVrouterName,
  kDbiDstIpAddr,
  kDbiMask,
  kDbiNextHopAddr,
  kDbiNwmName,
  kDbiMetric,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiFlags,
  kDbiValidMetric,
  kDbiCsRowstatus,
  kDbiCsMetric,
  kDbiStaticIpRouteNumCols
};
}

namespace dhcprelay_server {
static const uint8_t kDhcpRelayServerNumPks = 3;
enum kDhcpRelayServerIndex {
  kDbiVtnName = 0,
  kDbiVrouterName,
  kDbiServerIpAddr,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiFlags,
  kDbiCsRowstatus,
  kDbiDhcpRelayServerNumCols
};
}

namespace dhcprelay_interface {
static const uint8_t kDhcpRelayIfNumPks = 3;
enum kDhcpRelayIfIndex {
  kDbiVtnName = 0,
  kDbiVrtName,
  kDbiIfName,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiFlags,
  kDbiCsRowstatus,
  kDbiDhcpRelayIfNumCols
};
}

namespace vbridge_networkmonitor_group {
static const uint8_t kVbrNwMonGrpNumPks = 3;
enum kVbrNwMonGrpIndex {
  kDbiVtnName = 0,
  kDbiVbrName,
  kDbiNwmName,
  kDbiAdminStatus,
  kDbiOperStatus,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiFlags,
  kDbiValidAdminStatus,
  kDbiValidOperStatus,
  kDbiCsRowstatus,
  kDbiCsAdminStatus,
  kDbiVbrNwMonGrpNumCols
};
}

namespace vbridge_networkmonitor_host {
static const uint8_t kVbrNwMonHostNumPks = 4;
enum kVbrNwMonHostIndex {
  kDbiVtnName = 0,
  kDbiVbrName,
  kDbiNwmName,
  kDbiHostAddress,
  kDbiHealthInterval,
  kDbiRecoveryInterval,
  kDbiFailureCount,
  kDbiRecoveryCount,
  kDbiWaitTime,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiFlags,
  kDbiValidHealthInterval,
  kDbiValidRecoveryInterval,
  kDbiValidFailureCount,
  kDbiValidRecoveryCount,
  kDbiValidWaitTime,
  kDbiCsRowstatus,
  kDbiCsHealthInterval,
  kDbiCsRecoveryInterval,
  kDbiCsFailureCount,
  kDbiCsRecoveryCount,
  kDbiCsWaitTime,
  kDbiVbrNwMonHostNumCols
};
}

namespace vunknown {
static const uint8_t kVunknownNumPks = 2;
enum kVunknownIndex {
  kDbiVtnName = 0,
  kDbiVunknownName,
  kDbiDesc,
  kDbiType,
  kDbiDomainId,
  kDbiFlags,
  kDbiValidDesc,
  kDbiValidType,
  kDbiValidDomainId,
  kDbiCsRowstatus,
  kDbiCsDesc,
  kDbiCsType,
  kDbiCsDomainId,
  kDbiVunknownNumCols
};
}

namespace vunknown_interface {
static const uint8_t kVunknownIfNumPks = 3;
enum kVunknownIfIndex {
  kDbiVtnName = 0,
  kDbiVunknownName,
  kDbiIfName,
  kDbiDesc,
  kDbiAdminStatus,
  kDbiDomainId,
  kDbiFlags,
  kDbiValidDesc,
  kDbiValidAdminStatus,
  kDbiCsRowstatus,
  kDbiCsDesc,
  kDbiCsAdminStatus,
  kDbiVunknownIfNumCols
};
}

namespace vtep {
static const uint8_t kVtepNumPks = 2;
enum kVtepIndex {
  kDbiVtnName = 0,
  kDbiVtepName,
  kDbiDesc,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiOperStatus,
  kDbiDownCount,
  kDbiFlags,
  kDbiValidDesc,
  kDbiValidCtrlrName,
  kDbiValidDomainId,
  kDbiValidOperStatus,
  kDbiCsRowstatus,
  kDbiCsDesc,
  kDbiCsCtrlrName,
  kDbiCsDomainId,
  kDbiVtepNumCols
};
}

namespace vtep_interface {
static const uint8_t kVtepIfNumPks = 3;
enum kVtepIfIndex {
  kDbiVtnName = 0,
  kDbiVtepName,
  kDbiIfName,
  kDbiDesc,
  kDbiAdminStatus,
  kDbiLogicalPortId,
  kDbiVlanId,
  kDbiTagged,
  kDbiOperStatus,
  kDbiDownCount,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiFlags,
  kDbiValidDesc,
  kDbiValidAdminStatus,
  kDbiValidPortMap,
  kDbiValidLogicalPortId,
  kDbiValidVlanid,
  kDbiValidTagged,
  kDbiValidOperStatus,
  kDbiCsRowstatus,
  kDbiCsDesc,
  kDbiCsAdminStatus,
  kDbiCsPortMap,
  kDbiCsLogicalPortId,
  kDbiCsVlanid,
  kDbiCsTagged,
  kDbiVtepIfNumCols
};
}

namespace vtep_group {
static const uint8_t kVtepGrpNumPks = 2;
enum kVtepGrpIndex {
  kDbiVtnName = 0,
  kDbiVtepgrpName,
  kDbiCtrlrName,
  kDbiDesc,
  kDbiFlags,
  kDbiValidCtrlrName,
  kDbiValidDesc,
  kDbiCsRowstatus,
  kDbiCsCtrlrName,
  kDbiCsDesc,
  kDbiVtepGrpNumCols
};
}

namespace vtep_groupmember {
static const uint8_t kVtepGrpMemNumPks = 3;
enum kVtepGrpMemIndex {
  kDbiVtnName = 0,
  kDbiVtepgrpName,
  kDbiVtepgrpMemberName,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiFlags,
  kDbiCsRowstatus,
  kDbiVtepGrpMemNumCols
};
}

namespace vtunnel {
static const uint8_t kVtunnelNumPks = 2;
enum kVtunnelIndex {
  kDbiVtnName = 0,
  kDbiVtunnelName,
  kDbiDesc,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiUnderlayVtnName,
  kDbiVtepgrpName,
  kDbiLabel,
  kDbiOperStatus,
  kDbiDownCount,
  kDbiFlags,
  kDbiValidDesc,
  kDbiValidCtrlrName,
  kDbiValidDomainId,
  kDbiValidUnderlayVtnName,
  kDbiValidVtepgrpName,
  kDbiValidLabel,
  kDbiValidOperStatus,
  kDbiCsRowstatus,
  kDbiCsDesc,
  kDbiCsCtrlrName,
  kDbiCsDomainId,
  kDbiCsUnderlayVtnName,
  kDbiCsVtepgrpName,
  kDbiCsLabel,
  kDbiVtunnelNumCols
};
}

namespace vtunnel_interface {
static const uint8_t kVtunnelIfNumPks = 3;
enum kVtunnelIfIndex {
  kDbiVtnName = 0,
  kDbiVtunnelName,
  kDbiIfName,
  kDbiDesc,
  kDbiAdminStatus,
  kDbiLogicalPortId,
  kDbiVlanId,
  kDbiTagged,
  kDbiOperStatus,
  kDbiDownCount,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiFlags,
  kDbiValidDesc,
  kDbiValidAdminStatus,
  kDbiValidPortMap,
  kDbiValidLogicalPortId,
  kDbiValidVlanid,
  kDbiValidTagged,
  kDbiValidOperStatus,
  kDbiCsRowstatus,
  kDbiCsDesc,
  kDbiCsAdminStatus,
  kDbiCsPortMap,
  kDbiCsLogicalPortId,
  kDbiCsVlanid,
  kDbiCsTagged,
  kDbiVtunnelIfNumCols
};
}
// L2/L3 MoMgr Enum End

// POM MoMgr Enum Start

// Enum definition for index in global flowlist table Schema
namespace flowlist {
static const uint8_t kFlowListNumPks = 1;
enum FlowListSchema {
  kDbiFlowListName = 0,
  kDbiIpType,
  kDbiFlags,
  kDbiValidIpType,
  kDbiCsRowStatus,
  kDbiCsIpType,
  kDbiFlowListNumCols    // Number of Columns
};  // enum FlowListSchema
}  // namespace flowlist

// Enum definition for index in global flowlist controller table Schema
namespace flowlist_ctrlr {
static const uint8_t kFlowListCtrlrNumPks = 2;
enum FlowListCtrlrSchema {
  kDbiFlowListName = 0,
  kDbiCtrlrName,
  kDbiRefCount,
  kDbiFlags,
  kDbiValidIpType,
  kDbiCsRowStatus,
  kDbiCsIpType,
  kDbiFlowListCtrlrNumCols  // Number of Columns
};  // enum FlowListCtrlrSchema
}  // namespace flowlist_ctrlr

// Enum definition for index in global flowlist rename table Schema
namespace flowlist_rename {
static const uint8_t kFlowListRenameNumPks = 2;
enum FlowListRenameSchema {
  kDbiFlowListNameCtrlr = 0,
  kDbiCtrlrName,
  kDbiFlowListName,
  kDbiFlowListRenameNumCols  // Number of Columns
};  // enum FlowListRenameSchema
}  // namespace flowlist_rename


// Enum definition for index in global flowlist entry table Schema
namespace flowlist_entry {
static const uint8_t kFlowListEntryNumPks = 2;
enum FlowListEntrySchema {
  kDbiFlowListName = 0,
  kDbiSequenceNum,
  kDbiMacDst,
  kDbiMacSrc,
  kDbiMacEthType,
  kDbiDstIp,
  kDbiDstIpPrefix,
  kDbiSrcIp,
  kDbiSrcIpPrefix,
  kDbiVlanPriority,
  kDbiDstIpV6,
  kDbiDstIpV6Prefix,
  kDbiSrcIpV6,
  kDbiSrcIpV6Prefix,
  kDbiIpProtocol,
  kDbiIpDscp,
  kDbiL4DstPort,
  kDbiL4DstPortEndpt,
  kDbiL4SrcPort,
  kDbiL4SrcPortEndpt,
  kDbiIcmpType,
  kDbiIcmpCode,
  kDbiIcmpV6Type,
  kDbiIcmpV6Code,
  kDbiFlags,
  kDbiValidMacDst,
  kDbiValidMacSrc,
  kDbiValidMacEthType,
  kDbiValidDstIp,
  kDbiValidDstIpPrefix,
  kDbiValidSrcIp,
  kDbiValidSrcIpPrefix,
  kDbiValidVlanPriority,
  kDbiValidDstIpV6,
  kDbiValidDstIpV6Prefix,
  kDbiValidSrcIpV6,
  kDbiValidSrcIpV6Prefix,
  kDbiValidIpProtocol,
  kDbiValidIpDscp,
  kDbiValidL4DstPort,
  kDbiValidL4DstPortEndpt,
  kDbiValidL4SrcPort,
  kDbiValidL4SrcPortEndpt,
  kDbiValidIcmpType,
  kDbiValidIcmpCode,
  kDbiValidIcmpV6Type,
  kDbiValidIcmpV6Code,
  kDbiCsRowStatus,
  kDbiCsMacDst,
  kDbiCsMacSrc,
  kDbiCsMacEthType,
  kDbiCsDstIp,
  kDbiCsDstIpPrefix,
  kDbiCsSrcIp,
  kDbiCsSrcIpPrefix,
  kDbiCsVlanPriority,
  kDbiCsDstIpV6,
  kDbiCsDstIpV6Prefix,
  kDbiCsSrcIpV6,
  kDbiCsSrcIpV6Prefix,
  kDbiCsIpProtocol,
  kDbiCsIpDscp,
  kDbiCsL4DstPort,
  kDbiCsL4DstPortEndpt,
  kDbiCsL4SrcPort,
  kDbiCsL4SrcPortEndpt,
  kDbiCsIcmpType,
  kDbiCsIcmpCode,
  kDbiCsIcmpV6Type,
  kDbiCsIcmpV6Code,
  kDbiFlowListEntryNumCols  // Number of Columns
};  // FlowFilterEntrySchema
}  // namespace flowlist_entry

// Enum definition for index in global flowlist entry controller table Schema
namespace flowlist_entry_ctrlr {
static const uint8_t kFlowListEntryCtrlrNumPks = 3;
enum FlowListEntryCtrlrSchema {
  kDbiFlowListName = 0,
  kDbiSequenceNum,
  kDbiCtrlrName,
  kDbiFlags,
  kDbiValidMacDst,
  kDbiValidMacSrc,
  kDbiValidMacEthType,
  kDbiValidDstIp,
  kDbiValidDstIpPrefix,
  kDbiValidSrcIp,
  kDbiValidSrcIpPrefix,
  kDbiValidVlanPriority,
  kDbiValidDstIpV6,
  kDbiValidDstIpV6Prefix,
  kDbiValidSrcIpV6,
  kDbiValidSrcIpV6Prefix,
  kDbiValidIpProtocol,
  kDbiValidIpDscp,
  kDbiValidL4DstPort,
  kDbiValidL4DstPortEndpt,
  kDbiValidL4SrcPort,
  kDbiValidL4SrcPortEndpt,
  kDbiValidIcmpType,
  kDbiValidIcmpCode,
  kDbiValidIcmpV6Type,
  kDbiValidIcmpV6Code,
  kDbiCsRowStatus,
  kDbiCsMacDst,
  kDbiCsMacSrc,
  kDbiCsMacEthType,
  kDbiCsDstIp,
  kDbiCsDstIpPrefix,
  kDbiCsSrcIp,
  kDbiCsSrcIpPrefix,
  kDbiCsVlanPriority,
  kDbiCsDstIpV6,
  kDbiCsDstIpV6Prefix,
  kDbiCsSrcIpV6,
  kDbiCsSrcIpV6Prefix,
  kDbiCsIpProtocol,
  kDbiCsIpDscp,
  kDbiCsL4DstPort,
  kDbiCsL4DstPortEndpt,
  kDbiCsL4SrcPort,
  kDbiCsL4SrcPortEndpt,
  kDbiCsIcmpType,
  kDbiCsIcmpCode,
  kDbiCsIcmpV6Type,
  kDbiCsIcmpV6Code,
  kDbiFlowListEntryCtrlrNumCols  // Number of Columns
};  // FlowFilterEntryCtrlrSchema
}  // namespace flowlist_entry_Ctrlr

// Enum definition for index in global policing profile table Schema
namespace policingprofile {
static const uint8_t kPolicingProfileNumPks = 1;
enum PolicingProfileSchema {
  kDbiPolicingProfileName = 0,
  kDbiFlags,
  kDbiCsRowStatus,
  kDbiPolicingProfileNumCols  // Number of Columns
};  // PolicingProfileSchema
}  // namespace policingprofile

// Enum definition for index in global policing profile controller table Schema
namespace policingprofile_ctrlr {
static const uint8_t kPolicingProfileCtrlrNumPks = 2;
enum PolicingProfileCtrlrSchema {
  kDbiPolicingProfileName = 0,
  kDbiCtrlrName,
  kDbiRefCount,
  kDbiFlags,
  kDbiCsRowStatus,
  kDbiPolicingProfileCtrlrNumCols  // Number of Columns
};  // PolicingProfileCtrlrSchema
}  // namespace policingprofile_ctrlr

// Enum definition for index in global policing profile rename table Schema
namespace policingprofile_rename {
static const uint8_t kPolicingProfileRenameNumPks = 2;
enum PolicingProfileRenameSchema {
  kDbiPolicingProfileNameCtrlr = 0,
  kDbiCtrlrName,
  kDbiPolicingProfileName,
  kDbiPolicingProfileRenameNumCols  // Number of Columns
};  // PolicingProfileRenameSchema
}  // namespace policingprofile_rename


// Enum definition for index in global policing profile entry table Schema
namespace policingprofile_entry {
static const uint8_t kPolicingProfileEntryNumPks = 2;
enum PolicingProfileEntrySchema {
  kDbiPolicingProfileName = 0,
  kDbiSequenceNum,
  kDbiFlowlist,
  kDbiRate,
  kDbiCir,
  kDbiCbs,
  kDbiPir,
  kDbiPbs,
  kDbiGreenAction,
  kDbiGreenPriority,
  kDbiGreenDscp,
  kDbiGreenDrop,
  kDbiYellowAction,
  kDbiYellowPriority,
  kDbiYellowDscp,
  kDbiYellowDrop,
  kDbiRedAction,
  kDbiRedPriority,
  kDbiRedDscp,
  kDbiRedDrop,
  kDbiFlags,
  kDbiValidFlowlist,
  kDbiValidRate,
  kDbiValidCir,
  kDbiValidCbs,
  kDbiValidPir,
  kDbiValidPbs,
  kDbiValidGreenAction,
  kDbiValidGreenPriority,
  kDbiValidGreenDscp,
  kDbiValidGreenDrop,
  kDbiValidYellowAction,
  kDbiValidYellowPriority,
  kDbiValidYellowDscp,
  kDbiValidYellowDrop,
  kDbiValidRedAction,
  kDbiValidRedPriority,
  kDbiValidRedDscp,
  kDbiValidRedDrop,
  kDbiCsRowStatus,
  kDbiCsFlowlist,
  kDbiCsRate,
  kDbiCsCir,
  kDbiCsCbs,
  kDbiCsPir,
  kDbiCsPbs,
  kDbiCsGreenAction,
  kDbiCsGreenPriority,
  kDbiCsGreenDscp,
  kDbiCsGreenDrop,
  kDbiCsYellowAction,
  kDbiCsYellowPriority,
  kDbiCsYellowDscp,
  kDbiCsYellowDrop,
  kDbiCsRedAction,
  kDbiCsRedPriority,
  kDbiCsRedDscp,
  kDbiCsRedDrop,
  kDbiPolicingProfileEntryNumCols  // Number of Columns
};  // PolicingProfileEntrySchema
}  // namespace policingprofile_entry

// Enum definition for index in global policing profile entry
// controller table Schema
namespace policingprofile_entry_ctrlr {
static const uint8_t kPolicingProfileEntryCtrlrNumPks = 3;
enum PolicingProfileEntryCtrlrSchema {
  kDbiPolicingProfileName = 0,
  kDbiSequenceNum,
  kDbiCtrlrName,
  kDbiFlags,
  kDbiValidFlowlist,
  kDbiValidRate,
  kDbiValidCir,
  kDbiValidCbs,
  kDbiValidPir,
  kDbiValidPbs,
  kDbiValidGreenAction,
  kDbiValidGreenPriority,
  kDbiValidGreenDscp,
  kDbiValidGreenDrop,
  kDbiValidYellowAction,
  kDbiValidYellowPriority,
  kDbiValidYellowDscp,
  kDbiValidYellowDrop,
  kDbiValidRedAction,
  kDbiValidRedPriority,
  kDbiValidRedDscp,
  kDbiValidRedDrop,
  kDbiCsRowStatus,
  kDbiCsFlowlist,
  kDbiCsRate,
  kDbiCsCir,
  kDbiCsCbs,
  kDbiCsPir,
  kDbiCsPbs,
  kDbiCsGreenAction,
  kDbiCsGreenPriority,
  kDbiCsGreenDscp,
  kDbiCsGreenDrop,
  kDbiCsYellowAction,
  kDbiCsYellowPriority,
  kDbiCsYellowDscp,
  kDbiCsYellowDrop,
  kDbiCsRedAction,
  kDbiCsRedPriority,
  kDbiCsRedDscp,
  kDbiCsRedDrop,
  kDbiPolicingProfileEntryCtrlrNumCols  // Number of Columns
};  // PolicingProfileEntryCtrlrSchema
}  // namespace policingprofile_entry_ctrlr


// Enum definition for index in vtn flow filter table Schema
namespace vtn_flowfilter {
static const uint8_t kVtnFlowFilterNumPks = 2;
enum VtnFlowFilterSchema {
  kDbiVtnName = 0,
  kDbiInputDirection,
  kDbiFlags,
  kDbiCsRowStatus,
  kDbiVtnFlowFilterNumCols  // Number of Columns
};  // VtnFlowFilterSchema
}  // namespace vtn_flowfilter

// Enum definition for index in vtn flow filter controller table Schema
namespace vtn_flowfilter_ctrlr {
static const uint8_t kVtnFlowFilterCtrlrNumPks = 4;
enum VtnFlowFilterCtrlrSchema {
  kDbiVtnName = 0,
  kDbiInputDirection,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiFlags,
  kDbiCsRowStatus,
  kDbiVtnFlowFilterCtrlrNumCols  // Number of Columns
};  // VtnFlowFilterCtrlrSchema
}  // namespace vtn_flowfilter_ctrlr

// Enum definition for index in vtn flow filter entry table Schema
namespace vtn_flowfilter_entry {
static const uint8_t kVtnFlowFilterEntryNumPks = 3;
enum VtnFlowFilterEntrySchema {
  kDbiVtnName = 0,
  kDbiInputDirection,
  kDbiSequenceNum,
  kDbiFlowlistName,
  kDbiAction,
  kDbiNwnName,
  kDbiDscp,
  kDbiPriority,
  kDbiFlags,
  kDbiValidFlowlistName,
  kDbiValidAction,
  kDbiValidNwnName,
  kDbiValidDscp,
  kDbiValidPriority,
  kDbiCsRowStatus,
  kDbiCsFlowlistName,
  kDbiCsAction,
  kDbiCsNwnName,
  kDbiCsDscp,
  kDbiCsPriority,
  kDbiVtnFlowFilterEntryNumCols  // Number of Columns
};  // VtnFlowFilterEntrySchema
}  // namespace vtn_flowfilter_entry

// Enum definition for index in vtn flow filter entry controller table Schema
namespace vtn_flowfilter_entry_ctrlr {
static const uint8_t kVtnFlowFilterEntryCtrlrNumPks = 5;
enum VtnFlowFilterEntryCtrlrSchema {
  kDbiVtnName = 0,
  kDbiInputDirection,
  kDbiSequenceNum,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiFlags,
  kDbiValidFlowlistName,
  kDbiValidAction,
  kDbiValidNwnName,
  kDbiValidDscp,
  kDbiValidPriority,
  kDbiCsRowStatus,
  kDbiCsFlowlistName,
  kDbiCsAction,
  kDbiCsNwnName,
  kDbiCsDscp,
  kDbiCsPriority,
  kDbiVtnFlowFilterEntryCtrlrNumCols  // Number of Columns
};  // VtnFlowFilterEntryCtrlrSchema
}  // namespace vtn_flowfilter_entry_ctrlr

// Enum definition for index in vBridge flow filter table Schema
namespace vbr_flowfilter {
static const uint8_t kVbrFlowFilterNumPks = 3;
enum VbrFlowFilterSchema {
  kDbiVtnName = 0,
  kDbiVbrName,
  kDbiInputDirection,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiFlags,
  kDbiCsRowStatus,
  kDbiVbrFlowFilterNumCols  // Number of Columns
};  // VbrFlowFilterSchema
}  // namespace vbr_flowfilter

// Enum definition for index in vBridge flow filter entry table Schema
namespace vbr_flowfilter_entry {
static const uint8_t kVbrFlowFilterEntryNumPks = 4;
enum VbrFlowFilterEntrySchema {
  kDbiVtnName = 0,
  kDbiVbrName,
  kDbiInputDirection,
  kDbiSequenceNum,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiFlowlistName,
  kDbiAction,
  kDbiRedirectNode,
  kDbiRedirectPort,
  kDbiModifyDstMac,
  kDbiModifySrcMac,
  kDbiNwmName,
  kDbiDscp,
  kDbiPriority,
  kDbiFlags,
  kDbiValidFlowlistName,
  kDbiValidAction,
  kDbiValidRedirectNode,
  kDbiValidRedirectPort,
  kDbiValidModifyDstMac,
  kDbiValidModifySrcMac,
  kDbiValidNwmName,
  kDbiValidDscp,
  kDbiValidPriority,
  kDbiCsRowStatus,
  kDbiCsFlowlistName,
  kDbiCsAction,
  kDbiCsRedirectNode,
  kDbiCsRedirectPort,
  kDbiCsModifyDstMac,
  kDbiCsModifySrcMac,
  kDbiCsNwmName,
  kDbiCsDscp,
  kDbiCsPriority,
  kDbiVbrFlowFilterEntryNumCols  // Number of Columns
};  // VbrFlowFilterEntrySchema
}  // namespace vbr_flowfilter_entry


// Enum definition for index in vBridge interface flow filter table Schema
namespace vbr_if_flowfilter {
static const uint8_t kVbrIfFlowFilterNumPks = 4;
enum VbrIfFlowFilterSchema {
  kDbiVtnName = 0,
  kDbiVbrName,
  kDbiVbrIfName,
  kDbiInputDirection,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiFlags,
  kDbiCsRowStatus,
  kDbiVbrIfFlowFilterNumCols  // Number of Columns
};  // VbrFlowFilterEntrySchema
}  // namespace vbr_if_flowfilter

// Enum definition for index in vBridge interface flow filter entry table Schema
namespace vbr_if_flowfilter_entry {
static const uint8_t kVbrIfFlowFilterEntryNumPks = 5;
enum VbrIfFlowFilterEntrySchema {
  kDbiVtnName = 0,
  kDbiVbrName,
  kDbiVbrIfName,
  kDbiInputDirection,
  kDbiSequenceNum,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiFlowlistName,
  kDbiAction,
  kDbiRedirectNode,
  kDbiRedirectPort,
  kDbiModifyDstMac,
  kDbiModifySrcMac,
  kDbiNwmName,
  kDbiDscp,
  kDbiPriority,
  kDbiFlags,
  kDbiValidFlowlistName,
  kDbiValidAction,
  kDbiValidRedirectNode,
  kDbiValidRedirectPort,
  kDbiValidModifyDstMac,
  kDbiValidModifySrcMac,
  kDbiValidNwmName,
  kDbiValidDscp,
  kDbiValidPriority,
  kDbiCsRowStatus,
  kDbiCsFlowlistName,
  kDbiCsAction,
  kDbiCsRedirectNode,
  kDbiCsRedirectPort,
  kDbiCsModifyDstMac,
  kDbiCsModifySrcMac,
  kDbiCsNwmName,
  kDbiCsDscp,
  kDbiCsPriority,
  kDbiVbrIfFlowFilterEntryNumCols  // Number of Columns
};  // VbrIfFlowFilterEntrySchema
}  // namespace vbr_if_flowfilter_entry


// Enum definition for index in vRouter interface flow filter table Schema
namespace vrt_if_flowfilter {
static const uint8_t kVrtIfFlowFilterNumPks = 4;
enum VrtIfFlowFilterSchema {
  kDbiVtnName = 0,
  kDbiVrtName,
  kDbiVrtIfName,
  kDbiInputDirection,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiFlags,
  kDbiCsRowStatus,
  kDbiVrtIfFlowFilterNumCols  // Number of Columns
};  // VrtIfFlowFilterSchema
}  // namespace vrt_if_flowfilter

// Enum definition for index in vRouter interface flow filter entry table Schema
namespace vrt_if_flowfilter_entry {
static const uint8_t kVrtIfFlowFilterEntryNumPks = 5;
enum VrtIfFlowFilterEntrySchema {
  kDbiVtnName = 0,
  kDbiVrtName,
  kDbiVrtIfName,
  kDbiInputDirection,
  kDbiSequenceNum,
  kDbiCtrlrName,
  kDbiDomainId,
  kDbiFlowlistName,
  kDbiAction,
  kDbiRedirectNode,
  kDbiRedirectPort,
  kDbiModifyDstMac,
  kDbiModifySrcMac,
  kDbiNwmName,
  kDbiDscp,
  kDbiPriority,
  kDbiFlags,
  kDbiValidFlowlistName,
  kDbiValidAction,
  kDbiValidRedirectNode,
  kDbiValidRedirectPort,
  kDbiValidModifyDstMac,
  kDbiValidModifySrcMac,
  kDbiValidNwmName,
  kDbiValidDscp,
  kDbiValidPriority,
  kDbiCsRowStatus,
  kDbiCsFlowlistName,
  kDbiCsAction,
  kDbiCsRedirectNode,
  kDbiCsRedirectPort,
  kDbiCsModifyDstMac,
  kDbiCsModifySrcMac,
  kDbiCsNwmName,
  kDbiCsDscp,
  kDbiCsPriority,
  kDbiVrtIfFlowFilterEntryNumCols  // Number of Columns
};  // VrtIfFlowFilterEntrySchema
}  // namespace vrt_if_flowfilter_entry


// Enum definition for index in vtn policing map table Schema
namespace vtn_policingmap {
static const uint8_t kVtnPolicingMapNumPks = 1;
enum VtnPolicingMapSchema {
  kDbiVtnName = 0,
  kDbiPolicername,
  kDbiFlags,
  kDbiValidPolicername,
  kDbiCsRowStatus,
  kDbiCsPolicername,
  kDbiVtnPolicingMapNumCols  // Number of Columns
};  // VtnPolicingMap
}  // namespace vtn_policingmap

// Enum definition for index in vtn policing map controller table Schema
namespace vtn_policingmap_ctrlr {
static const uint8_t kVtnPolicingMapCtrlrNumPks = 3;
enum VtnPolicingMapCtrlrSchema {
  kDbiVtnName = 0,
  kDbiCtrlrname,
  kDbiDomainId,
  kDbiPolicername,
  kDbiFlags,
  kDbiValidPolicername,
  kDbiCsRowStatus,
  kDbiCsPolicername,
  kDbiVtnPolicingMapCtrlrNumCols  // Number of Columns
};  // VtnPolicingMapCtrlr
}  // namespace vtn_policingmap_ctrlr


// Enum definition for index in vBridge policing map table Schema
namespace vbr_policingmap {
static const uint8_t kVbrPolicingMapNumPks = 2;
enum VbrPolicingMapSchema {
  kDbiVtnName = 0,
  kDbiVbrName,
  kDbiCtrlrname,
  kDbiDomainId,
  kDbiPolicername,
  kDbiFlags,
  kDbiValidPolicername,
  kDbiCsRowStatus,
  kDbiCsPolicername,
  kDbiVbrPolicingMapNumCols  // Number of Columns
};  // VbrPolicingMap
}  // namespace vbr_policingmap

// Enum definition for index in vBridge interface policing map table Schema
namespace vbr_if_policingmap {
static const uint8_t kVbrIfPolicingMapNumPks = 3;
enum VbrIfPolicingMapSchema {
  kDbiVtnName = 0,
  kDbiVbrName,
  kDbiVbrIfName,
  kDbiCtrlrname,
  kDbiDomainId,
  kDbiPolicername,
  kDbiFlags,
  kDbiValidPolicername,
  kDbiCsRowStatus,
  kDbiCsPolicername,
  kDbiVbrIfPolicingMapNumCols  // Number of Columns
};  // VbrIfPolicingMap
}  // namespace vbr_if_policingmap
// POM MoMgr Enum End

namespace ctrlr {
static const uint8_t kCtrlrNumPkCols = 1;
enum CtrlrSchema {
  kDbiCtrlrName = 0,
  kDbiCtrlrType,
  kDbiCtrlrVersion,
  kDbiAuditDone,
  kDbiConfigDone,
  kDbiInvalidConfig,
  kCtrlrNumCols
};  // CtrlrSchema
}  // namespace ctrlr

enum kDalTableIndex {
  // L2/L3 MoMgr Table Enum Start
  kDbiVtnTbl = 0,
  kDbiVtnCtrlrTbl,
  kDbiVtnRenameTbl,
  kDbiVbrTbl,
  kDbiVbrVlanMapTbl,
  kDbiVbrIfTbl,
  kDbiVrtTbl,
  kDbiVrtIfTbl,
  kDbiVNodeRenameTbl,
  kDbiVlinkTbl,
  kDbiVlinkRenameTbl,
  kDbiStaticIpRouteTbl,
  kDbiDhcpRelayServerTbl,
  kDbiDhcpRelayIfTbl,
  kDbiVbrNwMonTbl,
  kDbiVbrNwMonHostTbl,
  kDbiVunknownTbl,
  kDbiVunknownIfTbl,
  kDbiVtepTbl,
  kDbiVtepIfTbl,
  kDbiVtepGrpTbl,
  kDbiVtepGrpMemTbl,
  kDbiVtunnelTbl,
  kDbiVtunnelIfTbl,
  // L2/L3 MoMgr Table Enum End
  // POM MoMgr Table Enum Start
  kDbiFlowListTbl,
  kDbiFlowListCtrlrTbl,
  kDbiFlowListRenameTbl,
  kDbiFlowListEntryTbl,
  kDbiFlowListEntryCtrlrTbl,
  kDbiPolicingProfileTbl,
  kDbiPolicingProfileCtrlrTbl,
  kDbiPolicingProfileRenameTbl,
  kDbiPolicingProfileEntryTbl,
  kDbiPolicingProfileEntryCtrlrTbl,
  kDbiVtnFlowFilterTbl,
  kDbiVtnFlowFilterCtrlrTbl,
  kDbiVtnFlowFilterEntryTbl,
  kDbiVtnFlowFilterEntryCtrlrTbl,
  kDbiVbrFlowFilterTbl,
  kDbiVbrFlowFilterEntryTbl,
  kDbiVbrIfFlowFilterTbl,
  kDbiVbrIfFlowFilterEntryTbl,
  kDbiVrtIfFlowFilterTbl,
  kDbiVrtIfFlowFilterEntryTbl,
  kDbiVtnPolicingMapTbl,
  kDbiVtnPolicingMapCtrlrTbl,
  kDbiVbrPolicingMapTbl,
  kDbiVbrIfPolicingMapTbl,
  // POM MoMgr Table Enum End

  kDbiCtrlrTbl,
  kDalNumTables
};  // enum kDalTableIndex
  extern const DalTableSchema table_schema[];
};  // namespace table

/* Inline Functions for Table Schema */
inline const char *
TableName(uint16_t table_index) {
  return ((table_index < table::kDalNumTables) ?
           table::table_schema[table_index].table_name : NULL);
}

inline size_t
TableNumCols(uint16_t table_index) {
  return ((table_index < table::kDalNumTables) ?
           table::table_schema[table_index].num_columns : 0);
}

inline size_t
TableNumPkCols(uint16_t table_index) {
  return ((table_index < table::kDalNumTables) ?
           table::table_schema[table_index].num_pk_columns : 0);
}

/* Inline Functions for Column Schema */
inline const char *
ColumnName(uint16_t table_index, uint16_t column_index) {
  return ((table_index < table::kDalNumTables) ?
           ((column_index < table::table_schema[table_index].num_columns) ?
             table::table_schema[table_index].
             column_schema[column_index].column_name :
             NULL) :
           NULL);
}

inline SQLSMALLINT
ColumnDbDataTypeId(uint16_t table_index, uint16_t column_index) {
  return ((table_index < table::kDalNumTables) ?
           ((column_index < table::table_schema[table_index].num_columns) ?
             table::table_schema[table_index].
             column_schema[column_index].db_data_type_id :
             SQL_UNKNOWN_TYPE) :
           SQL_UNKNOWN_TYPE);
}

inline SQLSMALLINT
ColumnDalDataTypeId(uint16_t table_index, uint16_t column_index) {
  return ((table_index < table::kDalNumTables) ?
           ((column_index < table::table_schema[table_index].num_columns) ?
             table::table_schema[table_index].
             column_schema[column_index].dal_data_type_id :
             SQL_UNKNOWN_TYPE) :
           SQL_UNKNOWN_TYPE);
}

inline size_t
ColumnDbArraySize(uint16_t table_index, uint16_t column_index) {
  return ((table_index < table::kDalNumTables) ?
           ((column_index < table::table_schema[table_index].num_columns) ?
             table::table_schema[table_index].
             column_schema[column_index].db_array_size :
             0) :
           0);
}

inline bool
ColumnIsPKeyIndex(uint16_t table_index, uint16_t column_index) {
  return (column_index < TableNumPkCols(table_index));
}

};  // namespace schema

}  // namespace dal
}  // namespace upll
}  // namespace unc
#endif  // DAL_DAL_SCHEMA_HH_
