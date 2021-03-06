/*
 * Copyright (c) 2012-2014 NEC Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#include "pfc/log.h"
#include "vrt_if_flowfilter_entry_momgr.hh"
#include "unc/upll_errno.h"
#include "upll_validation.hh"
#include "unc/upll_ipc_enum.h"
#include "flowlist_momgr.hh"
#include "vrt_if_momgr.hh"
#include "uncxx/upll_log.hh"
#include "vbr_flowfilter_entry_momgr.hh"

namespace unc {
namespace upll {
namespace kt_momgr {

using unc::upll::ipc_util::IpcUtil;

#define FLOWLIST_RENAME_FLAG    0x04  //  For 3rd Bit
#define VTN_RENAME_FLAG         0x01  //  For first Bit
#define VBR_RENAME_FLAG         0x10  //  For 2nd Bit
#define VRT_RENAME_FLAG         0x02  //  For 2nd Bit
#define NO_VRT_RENAME_FLAG      ~VRT_RENAME_FLAG
#define VLINK_CONFIGURED 0x01
#define PORTMAP_CONFIGURED 0x02
#define VLINK_PORTMAP_CONFIGURED 0x03
#define SET_FLAG_VLINK 0x40
#define NO_FLAG_VLINK ~SET_FLAG_VLINK
#define SET_FLAG_PORTMAP 0x20
#define SET_FLAG_VLINK_PORTMAP 0x80
#define SET_FLAG_NO_VLINK_PORTMAP ~0x9F
#define FLOW_RENAME             0x04
#define NO_FLOWLIST_RENAME      ~FLOW_RENAME

BindInfo VrtIfFlowFilterEntryMoMgr::vrt_if_flowfilter_entry_bind_info[] = {
  { uudst::vrt_if_flowfilter_entry::kDbiVtnName, CFG_KEY,
    offsetof(key_vrt_if_flowfilter_entry_t,
             flowfilter_key.if_key.vrt_key.vtn_key.vtn_name),
    uud::kDalChar, (kMaxLenVtnName + 1) },
  { uudst::vrt_if_flowfilter_entry::kDbiVrtName, CFG_KEY,
    offsetof(key_vrt_if_flowfilter_entry_t,
             flowfilter_key.if_key.vrt_key.vrouter_name),
    uud::kDalChar, (kMaxLenVnodeName + 1) },
  { uudst::vrt_if_flowfilter_entry::kDbiVrtIfName, CFG_KEY,
    offsetof(key_vrt_if_flowfilter_entry_t, flowfilter_key.if_key.if_name),
    uud::kDalChar, (kMaxLenInterfaceName + 1) },
  { uudst::vrt_if_flowfilter_entry::kDbiInputDirection, CFG_KEY,
    offsetof(key_vrt_if_flowfilter_entry_t, flowfilter_key.direction),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiSequenceNum, CFG_KEY,
    offsetof(key_vrt_if_flowfilter_entry_t, sequence_num),
    uud::kDalUint16, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiCtrlrName, CK_VAL,
    offsetof(key_user_data_t, ctrlr_id),
    uud::kDalChar, (kMaxLenCtrlrId + 1) },
  { uudst::vrt_if_flowfilter_entry::kDbiDomainId, CK_VAL,
    offsetof(key_user_data, domain_id),
    uud::kDalChar, (kMaxLenDomainId + 1) },
  { uudst::vrt_if_flowfilter_entry::kDbiFlowlistName, CFG_VAL,
    offsetof(val_flowfilter_entry_t, flowlist_name),
    uud::kDalChar, (kMaxLenFlowListName + 1) },
  { uudst::vrt_if_flowfilter_entry::kDbiAction, CFG_VAL,
    offsetof(val_flowfilter_entry_t, action),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiRedirectNode, CFG_VAL,
    offsetof(val_flowfilter_entry_t, redirect_node),
    uud::kDalChar, (kMaxLenVnodeName + 1) },
  { uudst::vrt_if_flowfilter_entry::kDbiRedirectPort, CFG_VAL,
    offsetof(val_flowfilter_entry_t, redirect_port),
    uud::kDalChar, (kMaxLenInterfaceName + 1) },
  { uudst::vrt_if_flowfilter_entry::kDbiModifyDstMac, CFG_VAL,
    offsetof(val_flowfilter_entry_t, modify_dstmac),
    uud::kDalChar, 6},
  { uudst::vrt_if_flowfilter_entry::kDbiModifySrcMac, CFG_VAL,
    offsetof(val_flowfilter_entry_t, modify_srcmac),
    uud::kDalChar, 6},
  { uudst::vrt_if_flowfilter_entry::kDbiNwmName, CFG_VAL,
    offsetof(val_flowfilter_entry_t, nwm_name),
    uud::kDalChar, (kMaxLenNwmName + 1) },
  { uudst::vrt_if_flowfilter_entry::kDbiDscp, CFG_VAL,
    offsetof(val_flowfilter_entry_t, dscp),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiPriority, CFG_VAL,
    offsetof(val_flowfilter_entry_t, priority),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiFlags, CK_VAL,
    offsetof(key_user_data_t, flags),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiValidFlowlistName, CFG_META_VAL,
    offsetof(val_flowfilter_entry_t, valid[0]),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiValidAction, CFG_META_VAL,
    offsetof(val_flowfilter_entry_t, valid[1]),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiValidRedirectNode, CFG_META_VAL,
    offsetof(val_flowfilter_entry_t, valid[2]),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiValidRedirectPort, CFG_META_VAL,
    offsetof(val_flowfilter_entry_t, valid[3]),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiValidModifyDstMac, CFG_META_VAL,
    offsetof(val_flowfilter_entry_t, valid[4]),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiValidModifySrcMac, CFG_META_VAL,
    offsetof(val_flowfilter_entry_t, valid[5]),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiValidNwmName, CFG_META_VAL,
    offsetof(val_flowfilter_entry_t, valid[6]),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiValidDscp, CFG_META_VAL,
    offsetof(val_flowfilter_entry_t, valid[7]),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiValidPriority, CFG_META_VAL,
    offsetof(val_flowfilter_entry_t, valid[8]),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiCsRowStatus, CS_VAL,
    offsetof(val_flowfilter_entry_t, cs_row_status),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiCsFlowlistName, CS_VAL,
    offsetof(val_flowfilter_entry_t, cs_attr[0]),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiCsAction, CS_VAL,
    offsetof(val_flowfilter_entry_t, cs_attr[1]),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiCsRedirectNode, CS_VAL,
    offsetof(val_flowfilter_entry_t, cs_attr[2]),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiCsRedirectPort, CS_VAL,
    offsetof(val_flowfilter_entry_t, cs_attr[3]),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiCsModifyDstMac, CS_VAL,
    offsetof(val_flowfilter_entry_t, cs_attr[4]),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiCsModifySrcMac, CS_VAL,
    offsetof(val_flowfilter_entry_t, cs_attr[5]),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiCsNwmName, CS_VAL,
    offsetof(val_flowfilter_entry_t, cs_attr[6]),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiCsDscp, CS_VAL,
    offsetof(val_flowfilter_entry_t, cs_attr[7]),
    uud::kDalUint8, 1 },
  { uudst::vrt_if_flowfilter_entry::kDbiCsPriority, CS_VAL,
    offsetof(val_flowfilter_entry_t, cs_attr[8]),
    uud::kDalUint8, 1 }
};

BindInfo VrtIfFlowFilterEntryMoMgr::vrt_if_flowfilter_entry_maintbl_bind_info[]
= {
  { uudst::vrt_if_flowfilter_entry::kDbiVtnName, CFG_MATCH_KEY,
    offsetof(key_vrt_if_flowfilter_entry_t,
             flowfilter_key.if_key.vrt_key.vtn_key.vtn_name),
    uud::kDalChar, (kMaxLenVtnName + 1) },
  { uudst::vrt_if_flowfilter_entry::kDbiVrtName, CFG_MATCH_KEY,
    offsetof(key_vrt_if_flowfilter_entry_t,
             flowfilter_key.if_key.vrt_key.vrouter_name),
    uud::kDalChar, (kMaxLenVnodeName + 1) },
  { uudst::vrt_if_flowfilter_entry::kDbiVtnName, CFG_INPUT_KEY,
    offsetof(key_rename_vnode_info_t, new_unc_vtn_name),
    uud::kDalChar, (kMaxLenVtnName + 1) },
  { uudst::vrt_if_flowfilter_entry::kDbiVrtName, CFG_INPUT_KEY,
    offsetof(key_rename_vnode_info_t, new_unc_vnode_name),
    uud::kDalChar, (kMaxLenVnodeName + 1) },
  { uudst::vrt_if_flowfilter_entry::kDbiFlags, CK_VAL,
    offsetof(key_user_data_t, flags),
    uud::kDalUint8, 1 }
};



VrtIfFlowFilterEntryMoMgr::VrtIfFlowFilterEntryMoMgr() : MoMgrImpl() {
  UPLL_FUNC_TRACE;
  //  Rename and ctrlr tables not required for this KT
  //  setting  table indexed for ctrl table and rename table to NULL
  ntable = (MAX_MOMGR_TBLS);
  table = new Table *[ntable];

  //  For Main Table
  table[MAINTBL] = new Table(
      uudst::kDbiVrtIfFlowFilterEntryTbl,
      UNC_KT_VRTIF_FLOWFILTER_ENTRY,
      vrt_if_flowfilter_entry_bind_info,
      IpctSt::kIpcStKeyVrtIfFlowfilterEntry,
      IpctSt::kIpcStValFlowfilterEntry,
      uudst::vrt_if_flowfilter_entry::kDbiVrtIfFlowFilterEntryNumCols);
  table[RENAMETBL] = NULL;
  table[CTRLRTBL] = NULL;
  nchild = 0;
  child = NULL;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::DeleteMo(IpcReqRespHeader *req,
                                              ConfigKeyVal *ikey,
                                              DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  uint8_t *ctrlr_id = NULL;
  upll_rc_t result_code = UPLL_RC_ERR_GENERIC;
  if (NULL == ikey && NULL == req) return result_code;
  result_code = ValidateMessage(req, ikey);
  if (result_code != UPLL_RC_SUCCESS)
    return result_code;

  result_code = UpdateConfigDB(ikey, req->datatype, UNC_OP_READ, dmi);
  if (UPLL_RC_ERR_INSTANCE_EXISTS != result_code) {
    UPLL_LOG_TRACE("Instance Does not exist");
    return result_code;
  }
  ConfigKeyVal *okey = NULL;
  result_code = GetChildConfigKey(okey, ikey);
  if (UPLL_RC_SUCCESS != result_code) {
    return result_code;
  }
  DbSubOp dbop = { kOpReadSingle, kOpMatchNone,
    kOpInOutCtrlr | kOpInOutFlag};
  result_code = ReadConfigDB(okey, req->datatype,
                             UNC_OP_READ, dbop, dmi, MAINTBL);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_DEBUG("Unable to read configuration from CandidateDb");
    delete okey;
    return result_code;
  }
  GET_USER_DATA_CTRLR(okey, ctrlr_id);
  val_flowfilter_entry_t *flowfilter_val =
      reinterpret_cast<val_flowfilter_entry_t *> (GetVal(okey));
  if (flowfilter_val->valid[UPLL_IDX_FLOWLIST_NAME_FFE] == UNC_VF_VALID) {
    uint8_t flag_vlink = 0;
    GET_USER_DATA_FLAGS(okey, flag_vlink);
    if (flag_vlink & SET_FLAG_VLINK) {
      FlowListMoMgr *mgr = reinterpret_cast<FlowListMoMgr *>
          (const_cast<MoManager *> (GetMoManager(UNC_KT_FLOWLIST)));
      result_code = mgr->AddFlowListToController(
          reinterpret_cast<char *>(flowfilter_val->flowlist_name), dmi,
          reinterpret_cast<char *>(ctrlr_id), req->datatype, UNC_OP_DELETE);
      if (result_code != UPLL_RC_SUCCESS) {
        DELETE_IF_NOT_NULL(okey);
        return result_code;
      }
    }
  }
  DELETE_IF_NOT_NULL(okey);
  result_code = UpdateConfigDB(ikey, req->datatype, UNC_OP_DELETE, dmi,
                               MAINTBL);

  return result_code;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::UpdateAuditConfigStatus(
    unc_keytype_configstatus_t cs_status,
    uuc::UpdateCtrlrPhase phase,
    ConfigKeyVal *&ckv_running,
    DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;

  val_flowfilter_entry_t *val = NULL;

  val = (ckv_running != NULL)?
      reinterpret_cast<val_flowfilter_entry_t *> (GetVal(ckv_running)):NULL;

  if (NULL == val) {
    UPLL_LOG_DEBUG("UpdateAuditConfigStatus::val Null");
    return UPLL_RC_ERR_GENERIC;
  }
  if (uuc::kUpllUcpCreate == phase )
    val->cs_row_status = cs_status;
  if ((uuc::kUpllUcpUpdate == phase) &&
      (val->cs_row_status == UNC_CS_INVALID ||
       val->cs_row_status == UNC_CS_NOT_APPLIED))
    val->cs_row_status = cs_status;
  for ( unsigned int loop = 0;
       loop < sizeof(val->valid)/sizeof(uint8_t); ++loop ) {
    if ((cs_status == UNC_CS_INVALID && UNC_VF_VALID == val->valid[loop]) ||
        cs_status == UNC_CS_APPLIED)
      val->cs_attr[loop] = cs_status;
  }
  UPLL_LOG_DEBUG("UpdateAuditConfigStatus::Success");
  return result_code;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::UpdateMo(IpcReqRespHeader *req,
                                              ConfigKeyVal *ikey,
                                              DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  ConfigKeyVal *okey = NULL;
  uint8_t *ctrlr_id = NULL;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  if (NULL == ikey && NULL == req) {
    UPLL_LOG_DEBUG("Both Request and Input Key are Null");
    return UPLL_RC_ERR_GENERIC;
  }
  result_code = ValidateMessage(req, ikey);
  if (UPLL_RC_SUCCESS != result_code) {
    UPLL_LOG_DEBUG("Vallidation is Failed %d", result_code);
    return result_code;
  }
  result_code = ValidateVrtIfValStruct(req, ikey);
  if (UPLL_RC_SUCCESS != result_code) {
    UPLL_LOG_DEBUG("ValidateVrtIfValStruct Failed %d", result_code);
    return result_code;
  }

  result_code = ValidateAttribute(ikey, dmi, req);
  if (UPLL_RC_SUCCESS != result_code)
    return result_code;
  //   Check and update the flowlist reference count
  //  if the flowlist object is referred
  FlowListMoMgr *flowlist_mgr = reinterpret_cast<FlowListMoMgr *>
      (const_cast<MoManager *> (GetMoManager(UNC_KT_FLOWLIST)));
  val_flowfilter_entry_t *flowfilter_val =
      reinterpret_cast<val_flowfilter_entry_t *> (GetVal(ikey));
  result_code = GetChildConfigKey(okey, ikey);
  if (UPLL_RC_SUCCESS != result_code) {
    UPLL_LOG_DEBUG("GetChildConfigKey failed");
    return result_code;
  }
  DbSubOp dbop = { kOpReadSingle, kOpMatchNone,
    kOpInOutCtrlr|kOpInOutDomain|kOpInOutFlag };
  result_code = ReadConfigDB(okey, UPLL_DT_CANDIDATE, UNC_OP_READ, dbop, dmi,
                             MAINTBL);
  if (UPLL_RC_SUCCESS != result_code) {
    delete okey;
    UPLL_LOG_DEBUG("ReadConfigDB failed %d", result_code);
    return result_code;
  }
  GET_USER_DATA_CTRLR(okey, ctrlr_id);
  uint8_t dbflag = 0;
  GET_USER_DATA_FLAGS(okey, dbflag);
  UPLL_LOG_DEBUG("CallingValidatecapanin VRTIF flowfilter ");
  result_code = ValidateCapability(req, ikey,
                                   reinterpret_cast<const char*>(ctrlr_id));
  if (result_code != UPLL_RC_SUCCESS) {
    delete okey;
    UPLL_LOG_DEBUG("Key/Attribute not supported by controller");
    return result_code;
  }

  if (UNC_VF_VALID == flowfilter_val->valid[UPLL_IDX_FLOWLIST_NAME_FFE] ||
      UNC_VF_VALID_NO_VALUE == flowfilter_val->
      valid[UPLL_IDX_FLOWLIST_NAME_FFE]) {
    if ((SET_FLAG_VLINK & dbflag)) {
      val_flowfilter_entry_t *temp_ffe_val = reinterpret_cast
          <val_flowfilter_entry_t *>(GetVal(okey));
      UPLL_LOG_DEBUG("flowlist name %s", flowfilter_val->flowlist_name);
      if (UNC_VF_VALID == flowfilter_val->valid[UPLL_IDX_FLOWLIST_NAME_VFFE] &&
          UNC_VF_VALID  == temp_ffe_val->
          valid[UPLL_IDX_FLOWLIST_NAME_VFFE]) {
        result_code = flowlist_mgr->AddFlowListToController(
            reinterpret_cast<char *>(temp_ffe_val->flowlist_name), dmi,
            reinterpret_cast<char *>(ctrlr_id), req->datatype, UNC_OP_DELETE);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("AddFlowListToController failed %d", result_code);
          delete okey;
          return result_code;
        }
        result_code = flowlist_mgr->AddFlowListToController(
            reinterpret_cast<char *>(flowfilter_val->flowlist_name), dmi,
            reinterpret_cast<char *>(ctrlr_id), req->datatype, UNC_OP_CREATE);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("AddFlowListToController failed %d", result_code);
          delete okey;
          return result_code;
        }
      } else if (UNC_VF_VALID == flowfilter_val->
                 valid[UPLL_IDX_FLOWLIST_NAME_VFFE] &&
                 (UNC_VF_INVALID == temp_ffe_val->
                  valid[UPLL_IDX_FLOWLIST_NAME_VFFE] || UNC_VF_VALID_NO_VALUE ==
                  temp_ffe_val->valid[UPLL_IDX_FLOWLIST_NAME_VFFE])) {
        result_code = flowlist_mgr->AddFlowListToController(
            reinterpret_cast<char *>(flowfilter_val->flowlist_name), dmi,
            reinterpret_cast<char *> (ctrlr_id), req->datatype, UNC_OP_CREATE);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("AddFlowListToController failed %d", result_code);
          delete okey;
          return result_code;
        }
      } else if (UNC_VF_VALID_NO_VALUE == flowfilter_val->
                 valid[UPLL_IDX_FLOWLIST_NAME_VFFE] &&
                 UNC_VF_VALID == temp_ffe_val->
                 valid[UPLL_IDX_FLOWLIST_NAME_VFFE]) {
        result_code = flowlist_mgr->AddFlowListToController(
            reinterpret_cast<char *>(temp_ffe_val->flowlist_name), dmi,
            reinterpret_cast<char *>(ctrlr_id), req->datatype, UNC_OP_DELETE);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("AddFlowListToController failed %d", result_code);
          delete okey;
          return result_code;
        }
      }
    }
  }
  DbSubOp dbop1 = {kOpNotRead, kOpMatchNone, kOpInOutNone};
  result_code = UpdateConfigDB(ikey, req->datatype, UNC_OP_UPDATE,
                               dmi, &dbop1, MAINTBL);
  if (UPLL_RC_SUCCESS != result_code) {
    UPLL_LOG_DEBUG("UpdateConfigDB is Failed %d", result_code);
    return result_code;
  }
  delete okey;
  return result_code;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::GetRenamedControllerKey(
    ConfigKeyVal *ikey, upll_keytype_datatype_t dt_type, DalDmlIntf *dmi,
    controller_domain *ctrlr_dom) {
  UPLL_FUNC_TRACE
      ConfigKeyVal *okey = NULL;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  //  ConfigKeyVal *temp_key=ikey;

  /* Get the controller's redirect node(vbridge/vrt) name -start*/
  val_flowfilter_entry_t *val_flowfilter_entry =
      reinterpret_cast<val_flowfilter_entry_t *> (GetVal(ikey));
  if (NULL == ctrlr_dom) {
    UPLL_LOG_DEBUG("ctrlr null");
    return UPLL_RC_ERR_GENERIC;
  }
  if (val_flowfilter_entry) {
    if ((UNC_VF_VALID ==
         val_flowfilter_entry->valid[UPLL_IDX_REDIRECT_NODE_FFE]) &&
        (UNC_VF_VALID ==
         val_flowfilter_entry->valid[UPLL_IDX_REDIRECT_PORT_FFE])) {
      unc_key_type_t child_key[]= { UNC_KT_VBRIDGE, UNC_KT_VROUTER };
      bool isRedirectVnodeVbridge = false;
      for (unsigned int i = 0;
           i < sizeof(child_key)/sizeof(child_key[0]); i++) {
        const unc_key_type_t ktype = child_key[i];
        MoMgrImpl *mgrvbr = reinterpret_cast<MoMgrImpl *>(
            const_cast<MoManager *>(GetMoManager(ktype)));
        if (!mgrvbr) {
          UPLL_LOG_DEBUG("mgrvbr failed");
          return UPLL_RC_ERR_GENERIC;
        }

        result_code = mgrvbr->GetChildConfigKey(okey, NULL);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("GetChildConfigKey fail");
          return result_code;
        }
        SET_USER_DATA_CTRLR_DOMAIN(okey, *ctrlr_dom);
        UPLL_LOG_DEBUG("ctrlr : %s; domain : %s", ctrlr_dom->ctrlr,
                       ctrlr_dom->domain);
        if (okey->get_key_type() == UNC_KT_VBRIDGE) {
          uuu::upll_strncpy(reinterpret_cast<key_vbr_t *>
                            (okey->get_key())->vbridge_name,
                            reinterpret_cast<val_flowfilter_entry_t *>
                            (ikey->get_cfg_val()->
                             get_val())->redirect_node, (kMaxLenVnodeName + 1));

          UPLL_LOG_DEBUG("redirect node vbr name (%s) (%s)",
                         reinterpret_cast<key_vbr_t *>
                         (okey->get_key())->vbridge_name,
                         reinterpret_cast<val_flowfilter_entry_t *>
                         (ikey->get_cfg_val()->
                          get_val())->redirect_node);
        } else if (okey->get_key_type() == UNC_KT_VROUTER) {
          uuu::upll_strncpy(reinterpret_cast<key_vrt_t *>
                            (okey->get_key())->vrouter_name,
                            reinterpret_cast<val_flowfilter_entry_t *>
                            (ikey->get_cfg_val()->
                             get_val())->redirect_node, (kMaxLenVnodeName + 1));

          UPLL_LOG_DEBUG("redirect node vrt name (%s) (%s)",
                         reinterpret_cast<key_vrt_t *>
                         (okey->get_key())->vrouter_name,
                         reinterpret_cast<val_flowfilter_entry_t *>
                         (ikey->get_cfg_val()->
                          get_val())->flowlist_name);
        }

        DbSubOp dbop = { kOpReadSingle,
          kOpMatchCtrlr | kOpMatchDomain,
          kOpInOutFlag };
        result_code = mgrvbr->ReadConfigDB(okey, dt_type, UNC_OP_READ,
                                           dbop, dmi, RENAMETBL);
        if (result_code != UPLL_RC_SUCCESS) {
          if (UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code) {
            UPLL_LOG_DEBUG("ReadConfigDB fail");
            DELETE_IF_NOT_NULL(okey);
            return result_code;
          }
        }

        if (result_code == UPLL_RC_SUCCESS) {
          val_rename_vnode *rename_val = NULL;
          isRedirectVnodeVbridge = true;
          rename_val = reinterpret_cast<val_rename_vnode *> (GetVal(okey));
          if (!rename_val) {
            UPLL_LOG_DEBUG("rename_val NULL.");
            DELETE_IF_NOT_NULL(okey);
            return UPLL_RC_ERR_GENERIC;
          }

          uuu::upll_strncpy(reinterpret_cast<val_flowfilter_entry_t*>
                            (ikey->get_cfg_val()->get_val())->redirect_node,
                            rename_val->ctrlr_vnode_name,
                            (kMaxLenVnodeName + 1));
        }
        DELETE_IF_NOT_NULL(okey);
        if (isRedirectVnodeVbridge)
          break;
      }
    }
  }
  /* -end*/
  UPLL_LOG_TRACE("%s GetRenamedCtrl vrt_if_ff_entry start",
                 ikey->ToStrAll().c_str());

  MoMgrImpl *VrtMoMgr = static_cast<MoMgrImpl*>
      ((const_cast<MoManager*> (GetMoManager(UNC_KT_VROUTER))));
  if (VrtMoMgr == NULL) {
    UPLL_LOG_DEBUG("InValid Reference of VRTIF");
    return UPLL_RC_ERR_BAD_REQUEST;
  }
  result_code = VrtMoMgr->GetChildConfigKey(okey, NULL);
  if ( result_code != UPLL_RC_SUCCESS ) {
    UPLL_LOG_DEBUG("GetChildConfigKey fail");
    return UPLL_RC_ERR_GENERIC;
  }
  SET_USER_DATA_CTRLR_DOMAIN(okey, *ctrlr_dom);

  UPLL_LOG_DEBUG("ctrlr : %s; domain : %s", ctrlr_dom->ctrlr,
                 ctrlr_dom->domain);


  strncpy(reinterpret_cast<char *>
          (reinterpret_cast<key_vrt *>(okey->get_key())->vtn_key.vtn_name),
          reinterpret_cast<const char *>
          (reinterpret_cast<key_vrt_if_flowfilter_entry_t *>
           (ikey->get_key())->flowfilter_key.if_key.vrt_key.vtn_key.vtn_name),
          kMaxLenVtnName + 1);
  UPLL_LOG_DEBUG("vrt name (%s) (%s)",
                 reinterpret_cast<char *>
                 (reinterpret_cast<key_vrt *>
                  (okey->get_key())->vtn_key.vtn_name),
                 reinterpret_cast<const char *>
                 (reinterpret_cast<key_vrt_if_flowfilter_entry_t *>
                  (ikey->get_key())->
                  flowfilter_key.if_key.vrt_key.vtn_key.vtn_name));
  strncpy(reinterpret_cast<char *>
          (reinterpret_cast<key_vrt *>(okey->get_key())->vrouter_name),
          reinterpret_cast<const char *>
          (reinterpret_cast<key_vrt_if_flowfilter_entry_t *>
           (ikey->get_key())->flowfilter_key.if_key.vrt_key.vrouter_name),
          kMaxLenVtnName + 1);
  UPLL_LOG_DEBUG("vrt name (%s) (%s)",
                 reinterpret_cast<char *>
                 (reinterpret_cast<key_vrt *>(okey->get_key())->vrouter_name),
                 reinterpret_cast<const char *>
                 (reinterpret_cast<key_vrt_if_flowfilter_entry_t *>
                  (ikey->get_key())->
                  flowfilter_key.if_key.vrt_key.vrouter_name));
  DbSubOp dbop = { kOpReadSingle,
    kOpMatchCtrlr | kOpMatchDomain,
    kOpInOutFlag };
  /* ctrlr_name */
  result_code =  VrtMoMgr->ReadConfigDB(okey, dt_type, UNC_OP_READ,
                                        dbop, dmi, RENAMETBL);
  if (( result_code != UPLL_RC_SUCCESS ) &&
      (UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code)) {
    UPLL_LOG_DEBUG("ReadConfigDB fail");
    DELETE_IF_NOT_NULL(okey);
    return UPLL_RC_ERR_GENERIC;
  }
  if (UPLL_RC_SUCCESS == result_code) {
    val_rename_vnode *rename_val =
        reinterpret_cast <val_rename_vnode *> (GetVal(okey));
    if (!rename_val) {
      UPLL_LOG_DEBUG("VRT Name is not Valid.");
      DELETE_IF_NOT_NULL(okey);
      return UPLL_RC_ERR_GENERIC;
    }

    uuu::upll_strncpy(
        reinterpret_cast<key_vrt_if_flowfilter_entry_t*>
        (ikey->get_key())->flowfilter_key.if_key.vrt_key.vtn_key.vtn_name,
        rename_val->ctrlr_vtn_name,
        (kMaxLenVtnName + 1));
    UPLL_LOG_DEBUG("vtn rename (%s) (%s)",
                   reinterpret_cast<key_vrt_if_flowfilter_entry_t*>
                   (ikey->get_key())->
                   flowfilter_key.if_key.vrt_key.vtn_key.vtn_name,
                   rename_val->ctrlr_vtn_name);

    uuu::upll_strncpy(
        reinterpret_cast<key_vrt_if_flowfilter_entry_t*>
        (ikey->get_key())->flowfilter_key.if_key.vrt_key.vrouter_name,
        rename_val->ctrlr_vnode_name,
        (kMaxLenVnodeName + 1));
    UPLL_LOG_DEBUG("vrt rename (%s) (%s)",
                   reinterpret_cast<key_vrt_if_flowfilter_entry_t*>
                   (ikey->get_key())->
                   flowfilter_key.if_key.vrt_key.vrouter_name,
                   rename_val->ctrlr_vnode_name);
  }
  DELETE_IF_NOT_NULL(okey);
  //   flowlist_name
  val_flowfilter_entry_t *val_ffe = reinterpret_cast
      <val_flowfilter_entry_t *>(GetVal(ikey));
  if (NULL == val_ffe) {
    UPLL_LOG_DEBUG("value structure is null");
    return UPLL_RC_SUCCESS;
  }
  if (strlen(reinterpret_cast<char *>
             (val_ffe->flowlist_name)) == 0) {
    return UPLL_RC_SUCCESS;
  }
  MoMgrImpl *mgrflist = static_cast<MoMgrImpl*>
      ((const_cast<MoManager*> (GetMoManager(UNC_KT_FLOWLIST))));
  result_code = mgrflist->GetChildConfigKey(okey, NULL);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_DEBUG("GetChildConfigKey fail");
    return result_code;
  }
  SET_USER_DATA_CTRLR(okey, ctrlr_dom->ctrlr);
  uuu::upll_strncpy(
      reinterpret_cast<key_flowlist_t *>(okey->get_key())->flowlist_name,
      reinterpret_cast<val_flowfilter_entry_t*>
      (ikey->get_cfg_val()->get_val())->flowlist_name,
      kMaxLenFlowListName + 1);
  UPLL_LOG_DEBUG("flowlist name (%s) (%s)",
                 reinterpret_cast<key_flowlist_t *>
                 (okey->get_key())->flowlist_name,
                 reinterpret_cast<val_flowfilter_entry_t*>
                 (ikey->get_cfg_val()->get_val())->flowlist_name);

  DbSubOp dbop1 = { kOpReadSingle, kOpMatchCtrlr, kOpInOutFlag };
  /* ctrlr_name */
  result_code =  mgrflist->ReadConfigDB(okey, dt_type,
                                        UNC_OP_READ, dbop1, dmi, RENAMETBL);
  if (( result_code != UPLL_RC_SUCCESS ) &&
      (UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code)) {
    UPLL_LOG_DEBUG("ReadConfigDB fail");
    DELETE_IF_NOT_NULL(okey);
    return UPLL_RC_ERR_GENERIC;
  }
  if (UPLL_RC_SUCCESS == result_code) {
    val_rename_flowlist_t *rename_val =
        reinterpret_cast <val_rename_flowlist_t*>(GetVal(okey));

    if (!rename_val) {
      UPLL_LOG_DEBUG("flowlist is not valid");
      DELETE_IF_NOT_NULL(okey);
      return UPLL_RC_ERR_GENERIC;
    }
    uuu::upll_strncpy(
        reinterpret_cast<val_flowfilter_entry_t*>
        (ikey->get_cfg_val()->get_val())->flowlist_name,
        rename_val->flowlist_newname,
        (kMaxLenFlowListName + 1));
  }
  DELETE_IF_NOT_NULL(okey);
  UPLL_LOG_TRACE("%s GetRenamedCtrl vrt_if_ff_entry end",
                 ikey->ToStrAll().c_str());
  UPLL_LOG_DEBUG("Renamed Controller key is sucessfull.");
  return UPLL_RC_SUCCESS;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::GetRenamedUncKey(
    ConfigKeyVal *ikey, upll_keytype_datatype_t dt_type, DalDmlIntf *dmi,
    uint8_t *ctrlr_id) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  UPLL_LOG_TRACE("%s GetRenamedUncKey vrtifff_entry start",
                 ikey->ToStrAll().c_str());
  if ((NULL == ikey) || (ctrlr_id == NULL) || (NULL == dmi)) {
    UPLL_LOG_DEBUG("ikey/ctrlr_id dmi NULL");
    return UPLL_RC_ERR_GENERIC;
  }

  ConfigKeyVal *unc_key = NULL;
  DbSubOp dbop = { kOpReadSingle, kOpMatchCtrlr, kOpInOutNone };
  MoMgrImpl *VrtIfMoMgr = static_cast<MoMgrImpl*>
      ((const_cast<MoManager*> (GetMoManager(UNC_KT_VROUTER))));
  if (VrtIfMoMgr == NULL) {
    UPLL_LOG_DEBUG("VrtMoMgr NULL");
    return UPLL_RC_ERR_GENERIC;
  }

  val_rename_vnode *rename_val = reinterpret_cast<val_rename_vnode*>
      (ConfigKeyVal::Malloc(sizeof(val_rename_vnode)));
  if (!rename_val) {
    UPLL_LOG_DEBUG("VrtMoMgr NULL");
    return UPLL_RC_ERR_GENERIC;
  }
  key_vrt_if_flowfilter_entry_t *ctrlr_key = reinterpret_cast
      <key_vrt_if_flowfilter_entry_t *> (ikey->get_key());
  if (!ctrlr_key) {
    UPLL_LOG_DEBUG("ctrlr_key NULL");
    free(rename_val);
    return UPLL_RC_ERR_GENERIC;
  }
  uuu::upll_strncpy(
      rename_val->ctrlr_vtn_name,
      ctrlr_key->flowfilter_key.if_key.vrt_key.vtn_key.vtn_name,
      (kMaxLenVtnName + 1));
  rename_val->valid[UPLL_CTRLR_VTN_NAME_VALID] = UNC_VF_VALID;

  uuu::upll_strncpy(rename_val->ctrlr_vnode_name,
                    ctrlr_key->flowfilter_key.if_key.vrt_key.vrouter_name,
                    (kMaxLenVnodeName + 1));
  rename_val->valid[UPLL_CTRLR_VNODE_NAME_VALID] = UNC_VF_VALID;

  result_code = VrtIfMoMgr->GetChildConfigKey(unc_key, NULL);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_DEBUG(" Failed to get ChildConfigkey structure");
    free(rename_val);
    VrtIfMoMgr = NULL;
    return result_code;
  }
  if (NULL == unc_key) {
    UPLL_LOG_DEBUG(" unc_key is NULL");
    free(rename_val);
    VrtIfMoMgr = NULL;
    return UPLL_RC_ERR_GENERIC;
  }

  SET_USER_DATA_CTRLR(unc_key, ctrlr_id);
  unc_key->AppendCfgVal(IpctSt::kIpcStValRenameVtn, rename_val);
  result_code = VrtIfMoMgr->ReadConfigDB(unc_key, dt_type,
                                         UNC_OP_READ, dbop, dmi, RENAMETBL);
  if (result_code == UPLL_RC_SUCCESS) {
    key_vrt_if_flowfilter_entry_t *key_vrt_if_flowfilter_entry =
        reinterpret_cast
        <key_vrt_if_flowfilter_entry_t *> (unc_key->get_key());
    uuu::upll_strncpy(
        ctrlr_key->flowfilter_key.if_key.vrt_key.vtn_key.vtn_name,
        key_vrt_if_flowfilter_entry->
        flowfilter_key.if_key.vrt_key.vtn_key.vtn_name,
        (kMaxLenVtnName + 1));
    uuu::upll_strncpy(
        ctrlr_key->flowfilter_key.if_key.vrt_key.vrouter_name,
        key_vrt_if_flowfilter_entry->
        flowfilter_key.if_key.vrt_key.vrouter_name,
        (kMaxLenVnodeName + 1));
  } else if (UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code) {
    UPLL_LOG_DEBUG("ReadConfigDB err");
    DELETE_IF_NOT_NULL(unc_key);
    VrtIfMoMgr = NULL;
    return result_code;
  }
  VrtIfMoMgr = NULL;
  DELETE_IF_NOT_NULL(unc_key);

  val_flowfilter_entry_t *val_flowfilter_entry = NULL;
  pfcdrv_val_flowfilter_entry_t *pfc_val_import = NULL;
  if (ikey->get_cfg_val() &&
      (ikey->get_cfg_val()->get_st_num() ==
       IpctSt::kIpcStPfcdrvValFlowfilterEntry)) {
    UPLL_LOG_DEBUG("val struct num (%d)", ikey->get_cfg_val()->get_st_num());
    pfc_val_import = reinterpret_cast<pfcdrv_val_flowfilter_entry_t *>
        (ikey->get_cfg_val()->get_val());
    val_flowfilter_entry = &pfc_val_import->val_ff_entry;
    UPLL_LOG_DEBUG("FLOWLIST name (%s)", val_flowfilter_entry->flowlist_name);
  } else if (ikey->get_cfg_val() &&
             (ikey->get_cfg_val()->get_st_num() ==
              IpctSt::kIpcStValFlowfilterEntry)) {
    val_flowfilter_entry = reinterpret_cast
        <val_flowfilter_entry_t *>(GetVal(ikey));
  }

  if (!val_flowfilter_entry) {
    UPLL_LOG_DEBUG("val_flowfilter_entry NULL");
    return UPLL_RC_SUCCESS;
  }

  if (UNC_VF_VALID == val_flowfilter_entry
      ->valid[UPLL_IDX_FLOWLIST_NAME_FFE]) {
    val_rename_flowlist_t *rename_flowlist =
        reinterpret_cast<val_rename_flowlist_t*>
        (ConfigKeyVal::Malloc(sizeof(val_rename_flowlist_t)));
    if (!rename_flowlist) {
      UPLL_LOG_DEBUG("rename_flowlist NULL");
      free(rename_flowlist);
      return UPLL_RC_ERR_GENERIC;
    }

    uuu::upll_strncpy(rename_flowlist->flowlist_newname,
                      val_flowfilter_entry->flowlist_name,
                      (kMaxLenFlowListName + 1));
    rename_flowlist->valid[UPLL_IDX_RENAME_FLOWLIST_RFL] = UNC_VF_VALID;

    MoMgrImpl* mgr =static_cast<MoMgrImpl*>
        ((const_cast<MoManager*> (GetMoManager(UNC_KT_FLOWLIST))));
    if (!mgr) {
      UPLL_LOG_DEBUG("mgr failed");
      if (rename_flowlist) free(rename_flowlist);
      return UPLL_RC_ERR_GENERIC;
    }

    result_code = mgr->GetChildConfigKey(unc_key, NULL);
    if (UPLL_RC_SUCCESS != result_code) {
      UPLL_LOG_DEBUG("GetChildConfigKey fail");
      free(rename_flowlist);
      mgr = NULL;
      return result_code;
    }
    if (!unc_key) {
      UPLL_LOG_DEBUG("unc_key NULL");
      free(rename_flowlist);
      mgr = NULL;
      return result_code;
    }
    SET_USER_DATA_CTRLR(unc_key, ctrlr_id);
    unc_key->AppendCfgVal(IpctSt::kIpcStValRenameFlowlist, rename_flowlist);
    result_code = mgr->ReadConfigDB(unc_key, dt_type, UNC_OP_READ, dbop, dmi,
                                    RENAMETBL);
    if (result_code == UPLL_RC_SUCCESS) {
      key_flowlist_t *key_flowlist = reinterpret_cast <key_flowlist_t *>
          (unc_key->get_key());
      uuu::upll_strncpy(val_flowfilter_entry->flowlist_name,
                        key_flowlist->flowlist_name,
                        (kMaxLenFlowListName + 1));
    } else if (UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code) {
      UPLL_LOG_DEBUG("ReadConfigDB err");
      DELETE_IF_NOT_NULL(unc_key);
      mgr = NULL;
      return result_code;
    }
    UPLL_LOG_DEBUG("Key is filled with UncKey Successfully %d", result_code);
    DELETE_IF_NOT_NULL(unc_key);
    mgr = NULL;
  }

  if ((UNC_VF_VALID ==
       val_flowfilter_entry->valid[UPLL_IDX_REDIRECT_NODE_FFE]) &&
      (UNC_VF_VALID ==
       val_flowfilter_entry->valid[UPLL_IDX_REDIRECT_PORT_FFE])) {
    unc_key_type_t child_key[]= { UNC_KT_VBRIDGE, UNC_KT_VROUTER };
    bool isRedirectVnodeVbridge = false;
    for (unsigned int i = 0;
         i < sizeof(child_key)/sizeof(child_key[0]); i++) {
      const unc_key_type_t ktype = child_key[i];
      MoMgrImpl *mgrvbr = reinterpret_cast<MoMgrImpl *>(
          const_cast<MoManager *>(GetMoManager(ktype)));
      if (!mgrvbr) {
        UPLL_LOG_TRACE("mgrvbr failed");
        return UPLL_RC_ERR_GENERIC;
      }
      val_rename_vnode *rename_val = reinterpret_cast<val_rename_vnode*>
          (ConfigKeyVal::Malloc(sizeof(val_rename_vnode)));
      if (!rename_val) {
        UPLL_LOG_TRACE("rename_val NULL");
        return UPLL_RC_ERR_GENERIC;
      }

      uuu::upll_strncpy(rename_val->ctrlr_vnode_name,
                        val_flowfilter_entry->redirect_node,
                        (kMaxLenVnodeName + 1));
      rename_val->valid[UPLL_CTRLR_VNODE_NAME_VALID] = UNC_VF_VALID;

      result_code = mgrvbr->GetChildConfigKey(unc_key, NULL);
      if (UPLL_RC_SUCCESS != result_code) {
        UPLL_LOG_DEBUG("GetChildConfigKey Returned an error");
        if (rename_val) free(rename_val);
        mgrvbr = NULL;
        return result_code;
      }
      SET_USER_DATA_CTRLR(unc_key, ctrlr_id);
      unc_key->AppendCfgVal(IpctSt::kIpcStValRenameVtn, rename_val);
      result_code = mgrvbr->ReadConfigDB(unc_key,
                                         dt_type,
                                         UNC_OP_READ,
                                         dbop,
                                         dmi,
                                         RENAMETBL);
      if ((UPLL_RC_SUCCESS != result_code) &&
          (UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code)) {
        UPLL_LOG_DEBUG("ReadConfigDB failed %d", result_code);
        DELETE_IF_NOT_NULL(unc_key);
        mgrvbr = NULL;
        return result_code;
      }

      if (result_code == UPLL_RC_SUCCESS) {
        if (unc_key->get_key_type() == UNC_KT_VBRIDGE) {
          isRedirectVnodeVbridge = true;
          key_vbr *vbr_key = reinterpret_cast<key_vbr *>(unc_key->get_key());
          uuu::upll_strncpy(val_flowfilter_entry->redirect_node,
                            vbr_key->vbridge_name,
                            (kMaxLenVnodeName + 1));
        } else if (unc_key->get_key_type() == UNC_KT_VROUTER) {
          key_vrt *vrt_key = reinterpret_cast<key_vrt *>(unc_key->get_key());
          uuu::upll_strncpy(val_flowfilter_entry->redirect_node,
                            vrt_key->vrouter_name,
                            (kMaxLenVnodeName + 1));
        }
      }
      DELETE_IF_NOT_NULL(unc_key);
      mgrvbr = NULL;
      if (isRedirectVnodeVbridge) {
        UPLL_LOG_DEBUG("RedirectVnode is Vbridge");
        break;
      }
    }
  }
  if (UPLL_RC_ERR_NO_SUCH_INSTANCE == result_code)
    result_code = UPLL_RC_SUCCESS;
  UPLL_LOG_TRACE("%s GetRenamedUncKey vbrifff_entry end",
                 ikey->ToStrAll().c_str());
  return result_code;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::GetChildConfigKey(
    ConfigKeyVal *&okey, ConfigKeyVal *parent_key) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  key_vrt_if_flowfilter_entry_t *vrt_if_ffe_key;
  void *pkey = NULL;

  if (parent_key == NULL) {
    vrt_if_ffe_key = reinterpret_cast<key_vrt_if_flowfilter_entry_t*>
        (ConfigKeyVal::Malloc(sizeof(key_vrt_if_flowfilter_entry_t)));
    //  If no direction is specified , 0xFE is filled to bind output direction
    vrt_if_ffe_key->flowfilter_key.direction = 0xFE;
    okey = new ConfigKeyVal(UNC_KT_VRTIF_FLOWFILTER_ENTRY,
                            IpctSt::kIpcStKeyVrtIfFlowfilterEntry,
                            vrt_if_ffe_key, NULL);
    UPLL_LOG_DEBUG("Parent Key Filled ");
    return UPLL_RC_SUCCESS;
  } else {
    pkey = parent_key->get_key();
  }
  if (!pkey) return UPLL_RC_ERR_GENERIC;

  if (okey) {
    if (okey->get_key_type() != UNC_KT_VRTIF_FLOWFILTER_ENTRY)
      return UPLL_RC_ERR_GENERIC;
  }

  if ((okey) && (okey->get_key())) {
    vrt_if_ffe_key = reinterpret_cast<key_vrt_if_flowfilter_entry_t *>
        (okey->get_key());
  } else {
    vrt_if_ffe_key = reinterpret_cast<key_vrt_if_flowfilter_entry_t *>
        (ConfigKeyVal::Malloc(sizeof(key_vrt_if_flowfilter_entry_t)));
    //  If no direction is specified , 0xFE is filled to bind output direction
    vrt_if_ffe_key->flowfilter_key.direction = 0xFE;
  }
  switch (parent_key->get_key_type()) {
    case UNC_KT_VTN:
      uuu::upll_strncpy(
          vrt_if_ffe_key->flowfilter_key.if_key.vrt_key.vtn_key.vtn_name,
          reinterpret_cast<key_vtn_t *>
          (pkey)->vtn_name,
          kMaxLenVtnName + 1);
      break;
    case UNC_KT_VROUTER:
      uuu::upll_strncpy(
          vrt_if_ffe_key->flowfilter_key.if_key.vrt_key.vtn_key.vtn_name,
          reinterpret_cast<key_vrt_t *>
          (pkey)->vtn_key.vtn_name,
          kMaxLenVtnName + 1);
      uuu::upll_strncpy(
          vrt_if_ffe_key->flowfilter_key.if_key.vrt_key.vrouter_name,
          reinterpret_cast<key_vrt_t *>
          (pkey)->vrouter_name,
          kMaxLenVnodeName + 1);
      break;
    case UNC_KT_VRT_IF:
      uuu::upll_strncpy(
          vrt_if_ffe_key->flowfilter_key.if_key.vrt_key.vtn_key.vtn_name,
          reinterpret_cast<key_vrt_if_t *>
          (pkey)->vrt_key.vtn_key.vtn_name,
          kMaxLenVtnName + 1);
      uuu::upll_strncpy(
          vrt_if_ffe_key->flowfilter_key.if_key.vrt_key.vrouter_name,
          reinterpret_cast<key_vrt_if_t *>
          (pkey)->vrt_key.vrouter_name,
          kMaxLenVnodeName + 1);
      uuu::upll_strncpy(vrt_if_ffe_key->flowfilter_key.if_key.if_name,
                        reinterpret_cast<key_vrt_if_t *>
                        (pkey)->if_name,
                        kMaxLenInterfaceName + 1);
      break;
    case UNC_KT_VRTIF_FLOWFILTER:
      uuu::upll_strncpy(
          vrt_if_ffe_key->flowfilter_key.if_key.vrt_key.vtn_key.vtn_name,
          reinterpret_cast<key_vrt_if_flowfilter_t *>
          (pkey)->if_key.vrt_key.vtn_key.vtn_name,
          kMaxLenVtnName + 1);
      uuu::upll_strncpy(
          vrt_if_ffe_key->flowfilter_key.if_key.vrt_key.vrouter_name,
          reinterpret_cast<key_vrt_if_flowfilter_t *>
          (pkey)->if_key.vrt_key.vrouter_name,
          kMaxLenVnodeName + 1);
      uuu::upll_strncpy(vrt_if_ffe_key->flowfilter_key.if_key.if_name,
                        reinterpret_cast<key_vrt_if_flowfilter_t *>
                        (pkey)->if_key.if_name,
                        kMaxLenInterfaceName + 1);
      vrt_if_ffe_key->flowfilter_key.direction =
          reinterpret_cast<key_vrt_if_flowfilter_t *>
          (pkey)->direction;
      break;
    case UNC_KT_VRTIF_FLOWFILTER_ENTRY:
      uuu::upll_strncpy(
          vrt_if_ffe_key->flowfilter_key.if_key.vrt_key.vtn_key.vtn_name,
          reinterpret_cast<key_vrt_if_flowfilter_entry_t *>
          (pkey)->flowfilter_key.if_key.vrt_key.vtn_key.vtn_name,
          kMaxLenVtnName + 1);
      uuu::upll_strncpy(
          vrt_if_ffe_key->flowfilter_key.if_key.vrt_key.vrouter_name,
          reinterpret_cast<key_vrt_if_flowfilter_entry_t *>
          (pkey)->flowfilter_key.if_key.vrt_key.vrouter_name,
          kMaxLenVnodeName + 1);
      uuu::upll_strncpy(vrt_if_ffe_key->flowfilter_key.if_key.if_name,
                        reinterpret_cast<key_vrt_if_flowfilter_entry_t *>
                        (pkey)->flowfilter_key.if_key.if_name,
                        kMaxLenInterfaceName + 1);
      vrt_if_ffe_key->flowfilter_key.direction =
          reinterpret_cast<key_vrt_if_flowfilter_entry_t *>
          (pkey)->flowfilter_key.direction;
      vrt_if_ffe_key->sequence_num =
          reinterpret_cast<key_vrt_if_flowfilter_entry_t *>
          (pkey)->sequence_num;
      break;
    case UNC_KT_VBR_NWMONITOR:
      uuu::upll_strncpy(
          vrt_if_ffe_key->flowfilter_key.if_key.vrt_key.vtn_key.vtn_name,
          reinterpret_cast<key_vrt_if_flowfilter_entry_t *>
          (pkey)->flowfilter_key.if_key.vrt_key.vtn_key.vtn_name,
          kMaxLenVtnName + 1);
      break;
    default:
      if (vrt_if_ffe_key) free(vrt_if_ffe_key);
      return UPLL_RC_ERR_GENERIC;
  }


  if ((okey) && !(okey->get_key())) {
    UPLL_LOG_DEBUG("okey not null and flow list name updated");
    okey->SetKey(IpctSt::kIpcStKeyVrtIfFlowfilterEntry, vrt_if_ffe_key);
  }

  if (!okey) {
    okey = new ConfigKeyVal(UNC_KT_VRTIF_FLOWFILTER_ENTRY,
                            IpctSt::kIpcStKeyVrtIfFlowfilterEntry,
                            vrt_if_ffe_key, NULL);
  }
  SET_USER_DATA(okey, parent_key);
  UPLL_LOG_DEBUG("GetChildConfigKey :: okey filled Succesfully ");
  return result_code;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::MergeValidate(unc_key_type_t keytype,
                                                   const char *ctrlr_id,
                                                   ConfigKeyVal *ikey,
                                                   DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  ConfigKeyVal *ckval = NULL;
  if (NULL == ctrlr_id) {
    UPLL_LOG_DEBUG("ctrlr_id NULL");
    return result_code;
  }
  result_code = GetChildConfigKey(ckval, NULL);
  if (UPLL_RC_SUCCESS != result_code) {
    UPLL_LOG_DEBUG("ckval fail");
    return result_code;
  }
  DbSubOp dbop = { kOpReadMultiple, kOpMatchNone, kOpInOutNone};
  result_code = ReadConfigDB(ckval, UPLL_DT_IMPORT,
                             UNC_OP_READ, dbop, dmi, MAINTBL);
  if (UPLL_RC_SUCCESS != result_code) {
    DELETE_IF_NOT_NULL(ckval);
    if (result_code != UPLL_RC_ERR_NO_SUCH_INSTANCE) {
      UPLL_LOG_DEBUG("ReadConfigDB fail");
      return result_code;
    }
    return UPLL_RC_SUCCESS;
  }

  while (NULL != ckval) {
    val_flowfilter_entry_t* val = reinterpret_cast<val_flowfilter_entry_t *>
        (GetVal(ckval));
    if (val->valid[UPLL_IDX_REDIRECT_NODE_FFE] ==
        UNC_VF_VALID) {
      if (val->valid[UPLL_IDX_REDIRECT_PORT_FFE] ==
          UNC_VF_VALID) {
        result_code = VerifyRedirectDestination(ckval, dmi, UPLL_DT_IMPORT);
        if (result_code != UPLL_RC_SUCCESS) {
          DELETE_IF_NOT_NULL(ckval);
          UPLL_LOG_DEBUG("redirect-destination node/interface doesn't exists");
          return UPLL_RC_ERR_MERGE_CONFLICT;
        }
      }
    }
    ckval = ckval->get_next_cfg_key_val();
  }

  DELETE_IF_NOT_NULL(ckval);
  MoMgrImpl *mgr =
      reinterpret_cast<MoMgrImpl *>(const_cast<MoManager *>(GetMoManager(
                  UNC_KT_VRTIF_FLOWFILTER)));
  if (!mgr) {
    UPLL_LOG_DEBUG("Invalid mgr param");
    return UPLL_RC_ERR_GENERIC;
  }

  result_code = mgr->MergeValidate(keytype, ctrlr_id, ikey, dmi);
  UPLL_LOG_DEBUG("MergeValidate result code (%d)", result_code);
  return result_code;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::DupConfigKeyVal(ConfigKeyVal *&okey,
                                                     ConfigKeyVal *&req,
                                                     MoMgrTables tbl) {
  UPLL_FUNC_TRACE;
  if (req == NULL) {
    UPLL_LOG_DEBUG("Request is null");
    return UPLL_RC_ERR_GENERIC;
  }
  if (okey != NULL) {
    UPLL_LOG_DEBUG("oKey already Contains Data");
    return UPLL_RC_ERR_GENERIC;
  }
  if (req->get_key_type() != UNC_KT_VRTIF_FLOWFILTER_ENTRY) {
    UPLL_LOG_DEBUG(" DupConfigKeyval Failed.");
    return UPLL_RC_ERR_GENERIC;
  }
  ConfigVal *tmp1 = NULL, *tmp = (req)->get_cfg_val();
  if (tmp) {
    if (tbl == MAINTBL) {
      val_flowfilter_entry_t *ival =
          reinterpret_cast <val_flowfilter_entry_t *>(GetVal(req));
      if (ival != NULL) {
        val_flowfilter_entry_t *val_ffe =
            reinterpret_cast<val_flowfilter_entry_t *>
            (ConfigKeyVal::Malloc(sizeof(val_flowfilter_entry_t)));

        memcpy(val_ffe, ival, sizeof(val_flowfilter_entry_t));
        tmp1 = new ConfigVal(IpctSt::kIpcStValFlowfilterEntry,
                             val_ffe);
      }
    }
  }

  void *tkey = (req != NULL) ? (req)->get_key() : NULL;
  key_vrt_if_flowfilter_entry_t *ikey =
      reinterpret_cast<key_vrt_if_flowfilter_entry_t *>(tkey);
  key_vrt_if_flowfilter_entry_t *key_vrt_if_ffe =
      reinterpret_cast<key_vrt_if_flowfilter_entry_t*>
      (ConfigKeyVal::Malloc(sizeof(key_vrt_if_flowfilter_entry_t)));

  memcpy(key_vrt_if_ffe, ikey, sizeof(key_vrt_if_flowfilter_entry_t));
  okey = new ConfigKeyVal(UNC_KT_VRTIF_FLOWFILTER_ENTRY,
                          IpctSt::kIpcStKeyVrtIfFlowfilterEntry,
                          key_vrt_if_ffe, tmp1);
  if (okey) {
    SET_USER_DATA(okey, req);
  }
  UPLL_LOG_DEBUG("DupConfigkeyVal Succesfull.");
  return UPLL_RC_SUCCESS;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::ReadMo(IpcReqRespHeader *req,
                                            ConfigKeyVal *ikey,
                                            DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  controller_domain ctrlr_dom;
  ConfigKeyVal* l_key = NULL, *dup_key = NULL;
  DbSubOp dbop1 = { kOpReadSingle, kOpMatchNone,
    kOpInOutCtrlr|kOpInOutDomain|kOpInOutFlag };
  result_code = ValidateMessage(req, ikey);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_DEBUG("ValidateMessage failed result_code %d", result_code);
    return result_code;
  }

  memset(&ctrlr_dom, 0, sizeof(controller_domain));
  switch (req->datatype) {
    //  Retrieving config information
    case UPLL_DT_CANDIDATE:
    case UPLL_DT_RUNNING:
    case UPLL_DT_STARTUP:
    case UPLL_DT_STATE:
      if (req->option1 == UNC_OPT1_NORMAL) {
        result_code = ReadInfoFromDB(req, ikey, dmi, &ctrlr_dom);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG(" Read request failed result(%d)", result_code);
          return result_code;
        }
        //  Retrieving state information
      } else if ((req->datatype == UPLL_DT_STATE) &&
                 (req->option1 == UNC_OPT1_DETAIL)&&
                 (req->option2 == UNC_OPT2_NONE)) {
        result_code =  DupConfigKeyVal(dup_key, ikey, MAINTBL);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("DupConfigKeyVal Faill in ReadMo for dup_key");
          return result_code;
        }

        result_code = ReadConfigDB(dup_key, req->datatype,
                                   UNC_OP_READ, dbop1, dmi, MAINTBL);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("ReadConfigDB  Faill in ReadMo for dup_key");
          delete dup_key;
          return result_code;
        }
        result_code =  DupConfigKeyVal(l_key, ikey, MAINTBL);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("DupConfigKeyVal Faill in ReadMo for l_key");
          delete dup_key;
          return result_code;
        }
        GET_USER_DATA_CTRLR_DOMAIN(dup_key, ctrlr_dom);
        SET_USER_DATA_CTRLR_DOMAIN(l_key, ctrlr_dom);

        // Added CapaCheck
        UPLL_LOG_DEBUG("Calling ValidateCapability From ReadMo ");
        result_code = ValidateCapability(req, ikey,
                                         reinterpret_cast<const char *>
                                         (ctrlr_dom.ctrlr));
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("validate Capability Failed %d", result_code);
          DELETE_IF_NOT_NULL(dup_key);
          DELETE_IF_NOT_NULL(l_key);
          return result_code;
        }

        //  1.Getting renamed name if renamed
        result_code = GetRenamedControllerKey(l_key, req->datatype,
                                              dmi, &ctrlr_dom);
        if (result_code != UPLL_RC_SUCCESS) {
          DELETE_IF_NOT_NULL(dup_key);
          DELETE_IF_NOT_NULL(l_key);
          return result_code;
        }

        uint8_t vlink_flag = 0;
        GET_USER_DATA_FLAGS(dup_key, vlink_flag);
        if (!(SET_FLAG_VLINK & vlink_flag)) {
          UPLL_LOG_DEBUG("Vlink Not Configured");
          DELETE_IF_NOT_NULL(dup_key);
          DELETE_IF_NOT_NULL(l_key);
          return UPLL_RC_ERR_NOT_ALLOWED_AT_THIS_TIME;
        }

        pfcdrv_val_flowfilter_entry_t *pfc_val =
            reinterpret_cast<pfcdrv_val_flowfilter_entry_t *>
            (ConfigKeyVal::Malloc(sizeof(pfcdrv_val_flowfilter_entry_t)));

        pfc_val->valid[PFCDRV_IDX_FLOWFILTER_ENTRY_FFE] = UNC_VF_INVALID;
        pfc_val->valid[PFCDRV_IDX_VAL_VBRIF_VEXTIF_FFE] = UNC_VF_VALID;
        pfc_val->val_vbrif_vextif.valid[PFCDRV_IDX_INTERFACE_TYPE] =
            UNC_VF_VALID;
        pfc_val->val_vbrif_vextif.valid[PFCDRV_IDX_VEXTERNAL_NAME_VBRIF] =
            UNC_VF_INVALID;
        pfc_val->val_vbrif_vextif.valid[PFCDRV_IDX_VEXT_IF_NAME_VBRIF] =
            UNC_VF_INVALID;
        pfc_val->val_vbrif_vextif.interface_type = PFCDRV_IF_TYPE_VBRIF;

        l_key->SetCfgVal(new ConfigVal(IpctSt::kIpcStPfcdrvValFlowfilterEntry,
                                       pfc_val));

        //  2.send request to driver
        IpcResponse ipc_resp;
        memset(&ipc_resp, 0, sizeof(IpcResponse));
        IpcRequest ipc_req;
        memset(&ipc_req, 0, sizeof(IpcRequest));
        ipc_req.header.clnt_sess_id = req->clnt_sess_id;
        ipc_req.header.config_id = req->config_id;
        ipc_req.header.operation = req->operation;
        ipc_req.header.option1 = req->option1;
        ipc_req.header.datatype = req->datatype;
        ipc_req.ckv_data = l_key;
        if (!IpcUtil::SendReqToDriver(
                (const char *)ctrlr_dom.ctrlr,
                reinterpret_cast<char *>(ctrlr_dom.domain),
                PFCDRIVER_SERVICE_NAME, PFCDRIVER_SVID_LOGICAL,
                &ipc_req, true, &ipc_resp)) {
          UPLL_LOG_DEBUG("SendReqToDriver failed for Key %d controller %s",
                         l_key->get_key_type(),
                         reinterpret_cast<char *>(ctrlr_dom.ctrlr));
          DELETE_IF_NOT_NULL(ipc_req.ckv_data);
          DELETE_IF_NOT_NULL(ipc_resp.ckv_data);
          DELETE_IF_NOT_NULL(dup_key);
          return UPLL_RC_ERR_GENERIC;
        }

        if (ipc_resp.header.result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("Driver response for Key %d controller %s result %d",
                         l_key->get_key_type(), ctrlr_dom.ctrlr,
                         ipc_resp.header.result_code);
          DELETE_IF_NOT_NULL(ipc_req.ckv_data);
          DELETE_IF_NOT_NULL(ipc_resp.ckv_data);
          DELETE_IF_NOT_NULL(dup_key);
          return ipc_resp.header.result_code;
        }
        ConfigKeyVal *okey = NULL;
        result_code = ConstructReadDetailResponse(dup_key,
                                                  ipc_resp.ckv_data,
                                                  &okey);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("ConstructReadDetailResponse error code (%d)",
                         result_code);
          DELETE_IF_NOT_NULL(ipc_req.ckv_data);
          DELETE_IF_NOT_NULL(ipc_resp.ckv_data);
          DELETE_IF_NOT_NULL(dup_key);
          DELETE_IF_NOT_NULL(okey);
          return result_code;
        } else {
          if (okey != NULL) {
            ikey->ResetWith(okey);
            DELETE_IF_NOT_NULL(okey);
          }
        }
        DELETE_IF_NOT_NULL(ipc_resp.ckv_data);
        DELETE_IF_NOT_NULL(dup_key);
        DELETE_IF_NOT_NULL(l_key);
      }
      break;
    default:
      UPLL_LOG_DEBUG("Operation Not Allowed");
      result_code = UPLL_RC_ERR_NOT_ALLOWED_FOR_THIS_DT;
  }
  return result_code;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::ReadSiblingMo(IpcReqRespHeader *req,
                                                   ConfigKeyVal *ikey,
                                                   bool begin,
                                                   DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  controller_domain ctrlr_dom;
  ConfigKeyVal *dup_key = NULL, *l_key = NULL, *tctrl_key = NULL;
  ConfigKeyVal *okey = NULL, *tmp_key = NULL, *flag_key = NULL;
  DbSubOp dbop = { kOpReadSingle, kOpMatchNone,
    kOpInOutCtrlr|kOpInOutDomain|kOpInOutFlag };

  result_code = ValidateMessage(req, ikey);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_DEBUG("ValidateMessage failed result_code %d",
                   result_code);
    return result_code;
  }


  memset(&ctrlr_dom, 0, sizeof(controller_domain));
  switch (req->datatype) {
    //  Retrieving config information
    case UPLL_DT_CANDIDATE:
    case UPLL_DT_RUNNING:
    case UPLL_DT_STARTUP:
    case UPLL_DT_STATE:
      if (req->option1 == UNC_OPT1_NORMAL) {
        result_code = ReadInfoFromDB(req, ikey, dmi, &ctrlr_dom);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG(" Read request failed result(%d)", result_code);
        }
      } else if ((req->datatype == UPLL_DT_STATE) &&
                 (req->option1 == UNC_OPT1_DETAIL) &&
                 (req->option2 == UNC_OPT2_NONE)) {
        result_code =  DupConfigKeyVal(tctrl_key, ikey, MAINTBL);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("DupConfigKeyVal Failed for tctrl_key");
          return result_code;
        }
        result_code = ReadInfoFromDB(req, tctrl_key, dmi, &ctrlr_dom);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("ReadInfoFromDB Fail in ReadSiblingMo for tctrl_key");
          return result_code;
        }

        result_code =  DupConfigKeyVal(dup_key, tctrl_key, MAINTBL);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG(" DupConfigKeyVal failed for dup_key%d ",
                         result_code);
          return result_code;
        }

        result_code = ReadConfigDB(dup_key, req->datatype, UNC_OP_READ,
                                   dbop, dmi, MAINTBL);
        if (UPLL_RC_SUCCESS != result_code) {
          UPLL_LOG_DEBUG("ReadConfigDb failed for tctrl_key err code(%d)",
                         result_code);
          return result_code;
        }

        result_code =  DupConfigKeyVal(l_key, ikey, MAINTBL);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("DupConfigKeyVal Faill in ReadSiblingMo for l_key");
          return result_code;
        }
        GET_USER_DATA_CTRLR_DOMAIN(dup_key, ctrlr_dom);
        SET_USER_DATA_CTRLR_DOMAIN(l_key, ctrlr_dom);
        // Added CapaCheck
        result_code = ValidateCapability(req, ikey,
                                         reinterpret_cast<const char *>
                                         (ctrlr_dom.ctrlr));
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("validate Capability Failed %d", result_code);
          return result_code;
        }

        //  1.Getting renamed name if renamed
        result_code = GetRenamedControllerKey(l_key, req->datatype,
                                              dmi, &ctrlr_dom);
        if (result_code != UPLL_RC_SUCCESS) {
          return result_code;
        }

        result_code =  DupConfigKeyVal(flag_key, tctrl_key, MAINTBL);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG(" DupConfigKeyVal failed for flag_key %d ",
                         result_code);
          return result_code;
        }

        DbSubOp dbop2 = { kOpReadSingle, kOpMatchNone, kOpInOutFlag };
        result_code = ReadConfigDB(flag_key, req->datatype ,
                                   UNC_OP_READ, dbop2, dmi, MAINTBL);
        if (UPLL_RC_ERR_NO_SUCH_INSTANCE == result_code) {
          UPLL_LOG_DEBUG("No Recrods in the Vrt_If_FlowFilter_Entry Table");
          return UPLL_RC_SUCCESS;
        }
        uint8_t vlink_flag = 0;
        GET_USER_DATA_FLAGS(flag_key, vlink_flag);
        if (!(SET_FLAG_VLINK & vlink_flag)) {
          UPLL_LOG_DEBUG("Vlink Not Configured");
          return UPLL_RC_ERR_NOT_ALLOWED_AT_THIS_TIME;
        }
        pfcdrv_val_flowfilter_entry_t *pfc_val =
            reinterpret_cast<pfcdrv_val_flowfilter_entry_t *>
            (ConfigKeyVal::Malloc(sizeof(pfcdrv_val_flowfilter_entry_t)));

        pfc_val->valid[PFCDRV_IDX_FLOWFILTER_ENTRY_FFE] = UNC_VF_INVALID;
        pfc_val->valid[PFCDRV_IDX_VAL_VBRIF_VEXTIF_FFE] = UNC_VF_VALID;
        pfc_val->val_vbrif_vextif.valid[PFCDRV_IDX_INTERFACE_TYPE] =
            UNC_VF_VALID;
        pfc_val->val_vbrif_vextif.valid[PFCDRV_IDX_VEXTERNAL_NAME_VBRIF] =
            UNC_VF_INVALID;
        pfc_val->val_vbrif_vextif.valid[PFCDRV_IDX_VEXT_IF_NAME_VBRIF] =
            UNC_VF_INVALID;
        pfc_val->val_vbrif_vextif.interface_type = PFCDRV_IF_TYPE_VBRIF;

        l_key->SetCfgVal(new ConfigVal(IpctSt::kIpcStPfcdrvValFlowfilterEntry,
                                       pfc_val));
        //  2.send request to driver
        IpcResponse ipc_resp;
        memset(&ipc_resp, 0, sizeof(IpcResponse));
        IpcRequest ipc_req;
        memset(&ipc_req, 0, sizeof(IpcRequest));
        ipc_req.header.clnt_sess_id = req->clnt_sess_id;
        ipc_req.header.config_id = req->config_id;
        ipc_req.header.operation = UNC_OP_READ;
        ipc_req.header.option1 = req->option1;
        ipc_req.header.datatype = req->datatype;
        tmp_key = tctrl_key;
        while (tmp_key !=NULL) {
          reinterpret_cast<key_vrt_if_flowfilter_entry_t*>
              (l_key->get_key())->flowfilter_key.direction =
              reinterpret_cast<key_vrt_if_flowfilter_entry_t*>
              (tmp_key->get_key())->flowfilter_key.direction;
          reinterpret_cast<key_vrt_if_flowfilter_entry_t*>
              (l_key->get_key())->sequence_num =
              reinterpret_cast<key_vrt_if_flowfilter_entry_t*>
              (tmp_key->get_key())->sequence_num;
          ipc_req.ckv_data = l_key;
          if (!IpcUtil::SendReqToDriver(
                  (const char *)ctrlr_dom.ctrlr,
                  reinterpret_cast<char *>(ctrlr_dom.domain),
                  PFCDRIVER_SERVICE_NAME, PFCDRIVER_SVID_LOGICAL,
                  &ipc_req, true, &ipc_resp)) {
            UPLL_LOG_DEBUG("SendReqToDriver failed for Key %d controller %s",
                           l_key->get_key_type(),
                           reinterpret_cast<char *>(ctrlr_dom.ctrlr));
            return UPLL_RC_ERR_GENERIC;
          }

          if (ipc_resp.header.result_code != UPLL_RC_SUCCESS) {
            UPLL_LOG_DEBUG("Driver response for Key %d controller"
                           "%s result %d",
                           l_key->get_key_type(), ctrlr_dom.ctrlr,
                           ipc_resp.header.result_code);
            return ipc_resp.header.result_code;
          }

          result_code = ConstructReadDetailResponse(tmp_key,
                                                    ipc_resp.ckv_data,
                                                    &okey);
          if (result_code != UPLL_RC_SUCCESS) {
            UPLL_LOG_DEBUG("ConstructReadDetailResponse error code (%d)",
                           result_code);
            return result_code;
          }
          tmp_key = tmp_key->get_next_cfg_key_val();
        }
        if ((okey != NULL) && (result_code == UPLL_RC_SUCCESS)) {
          ikey->ResetWith(okey);
        }
        DELETE_IF_NOT_NULL(l_key);
        DELETE_IF_NOT_NULL(tctrl_key);
      }
      break;
    default:
      result_code = UPLL_RC_ERR_NOT_ALLOWED_FOR_THIS_DT;
  }
  return result_code;
}

#if 0
upll_rc_t VrtIfFlowFilterEntryMoMgr::UpdateConfigStatus(
    ConfigKeyVal *key, unc_keytype_operation_t op, uint32_t driver_result,
    ConfigKeyVal *upd_key, DalDmlIntf *dmi, ConfigKeyVal *ctrlr_key) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;

  val_flowfilter_entry_t *vrtif_ff_entry_val = NULL;
  unc_keytype_configstatus_t cs_status =
      (driver_result == 0) ? UNC_CS_APPLIED : UNC_CS_NOT_APPLIED;

  vrtif_ff_entry_val = reinterpret_cast<val_flowfilter_entry_t *>
      (GetVal(key));

  if (vrtif_ff_entry_val == NULL) {
    UPLL_LOG_DEBUG("vrtif_ff_entry_val is Null");
    return UPLL_RC_ERR_GENERIC;
  }

  if (op == UNC_OP_CREATE) {
    if (vrtif_ff_entry_val->cs_row_status != UNC_CS_NOT_SUPPORTED)
      vrtif_ff_entry_val->cs_row_status = cs_status;
    for (unsigned int loop = 0;
         loop < (sizeof(vrtif_ff_entry_val->valid)/
                 sizeof(vrtif_ff_entry_val->valid[0])); ++loop) {
      if ((UNC_VF_VALID == vrtif_ff_entry_val->valid[loop])
          || (UNC_VF_VALID_NO_VALUE == vrtif_ff_entry_val->valid[loop]))
        if (vrtif_ff_entry_val->cs_attr[loop] != UNC_CS_NOT_SUPPORTED)
          vrtif_ff_entry_val->cs_attr[loop] =
              vrtif_ff_entry_val->cs_row_status;
    }
  } else if (op == UNC_OP_UPDATE) {
    void *fle_val1 = GetVal(key);
    void *fle_val2 = GetVal(upd_key);
    CompareValidValue(fle_val1, fle_val2, true);
    for (unsigned int loop = 0;
         loop < sizeof(vrtif_ff_entry_val->valid)/
         sizeof(vrtif_ff_entry_val->valid[0]); ++loop) {
      if (vrtif_ff_entry_val->cs_attr[loop] != UNC_CS_NOT_SUPPORTED)
        if ((UNC_VF_VALID == vrtif_ff_entry_val->valid[loop])
            ||(UNC_VF_VALID_NO_VALUE == vrtif_ff_entry_val->valid[loop]))
          //   if (CompareVal(vrtif_ff_entry_val, upd_key->GetVal()))
          vrtif_ff_entry_val->cs_attr[loop] =
              vrtif_ff_entry_val->cs_row_status;
    }
  } else {
    UPLL_LOG_DEBUG("Operation Not Supported.");
    return UPLL_RC_ERR_GENERIC;
  }
  UPLL_LOG_DEBUG("UpdateConfigStatus Success");
  return result_code;
}
#endif

bool VrtIfFlowFilterEntryMoMgr::IsValidKey(void *key,
                                           uint64_t index) {
  UPLL_FUNC_TRACE;
  upll_rc_t ret_val = UPLL_RC_SUCCESS;
  key_vrt_if_flowfilter_entry_t  *ff_key =
      reinterpret_cast<key_vrt_if_flowfilter_entry_t *>(key);
  if (ff_key == NULL)
    return false;

  switch (index) {
    case uudst::vrt_if_flowfilter_entry::kDbiVtnName:
      ret_val = ValidateKey(
          reinterpret_cast<char *>
          (ff_key->flowfilter_key.if_key.vrt_key.vtn_key.vtn_name),
          kMinLenVtnName, kMaxLenVtnName);
      if (ret_val != UPLL_RC_SUCCESS) {
        UPLL_LOG_DEBUG("VTN Name is not valid(%d)", ret_val);
        return false;
      }
      break;
    case uudst::vrt_if_flowfilter_entry::kDbiVrtName:
      ret_val = ValidateKey(reinterpret_cast<char *>
                            (ff_key->
                             flowfilter_key.if_key.vrt_key.vrouter_name),
                            kMinLenVnodeName, kMaxLenVnodeName);
      if (ret_val != UPLL_RC_SUCCESS) {
        UPLL_LOG_DEBUG("VRT Name is not valid(%d)", ret_val);
        return false;
      }
      break;
    case uudst::vrt_if_flowfilter_entry::kDbiVrtIfName:
      ret_val = ValidateKey(reinterpret_cast<char *>
                            (ff_key->flowfilter_key.if_key.if_name),
                            kMinLenInterfaceName, kMaxLenInterfaceName);
      if (ret_val != UPLL_RC_SUCCESS) {
        UPLL_LOG_DEBUG("VRTIF  Name is not valid(%d)", ret_val);
        return false;
      }
      break;
    case uudst::vrt_if_flowfilter_entry::kDbiInputDirection:
      if (ff_key->flowfilter_key.direction == 0xFE) {
        //  if operation is read sibling begin or
        //  read sibling count return false
        //  for output binding
        ff_key->flowfilter_key.direction = 0;
        return false;
      }
      if (!ValidateNumericRange(ff_key->flowfilter_key.direction,
                                (uint8_t) UPLL_FLOWFILTER_DIR_IN,
                                (uint8_t) UPLL_FLOWFILTER_DIR_OUT,
                                true, true)) {
        UPLL_LOG_DEBUG("direction syntax validation failed :");
        return false;
      }
      break;
    case uudst::vrt_if_flowfilter_entry::kDbiSequenceNum:
      if (!ValidateNumericRange(ff_key->sequence_num,
                                kMinFlowFilterSeqNum,
                                kMaxFlowFilterSeqNum,
                                true, true)) {
        UPLL_LOG_DEBUG("sequence number syntax validation failed");
        return false;
      }
      break;
    default:
      UPLL_LOG_DEBUG("Invalid Key Index");
      return false;
      break;
  }
  return true;
}


upll_rc_t VrtIfFlowFilterEntryMoMgr::IsReferenced(
    ConfigKeyVal *ikey,
    upll_keytype_datatype_t dt_type,
    DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_ERR_GENERIC;
  if (NULL == ikey) return UPLL_RC_ERR_BAD_REQUEST;
  DbSubOp dbop = { kOpReadExist, kOpMatchNone, kOpInOutNone };
  //  Check the object existence
  result_code = UpdateConfigDB(ikey,
                               dt_type,
                               UNC_OP_READ,
                               dmi,
                               &dbop,
                               MAINTBL);
  return result_code;
}


upll_rc_t VrtIfFlowFilterEntryMoMgr::ValidateAttribute(ConfigKeyVal *ikey,
                                                       DalDmlIntf *dmi,
                                                       IpcReqRespHeader *req) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;

  ConfigKeyVal *okey = NULL;
  if (!ikey || !ikey->get_key()) {
    UPLL_LOG_DEBUG("input key is null");
    return UPLL_RC_ERR_GENERIC;
  }
  MoMgrImpl *mgr = NULL;
  key_vrt_if_flowfilter_entry_t *key_vrtif_ffe =
      reinterpret_cast<key_vrt_if_flowfilter_entry_t *>(ikey->get_key());

  /* read val_flowfilter_entry from ikey*/
  val_flowfilter_entry_t *val_flowfilter_entry =
      static_cast<val_flowfilter_entry_t *>(
          ikey->get_cfg_val()->get_val());

  if (val_flowfilter_entry->valid[UPLL_IDX_FLOWLIST_NAME_FFE]
      == UNC_VF_VALID) {
    /* validate flowlist_name in val_flowfilter_entry exists in FLOWLIST table*/
    mgr = reinterpret_cast<MoMgrImpl *>
        (const_cast<MoManager *>(GetMoManager(UNC_KT_FLOWLIST)));

    if (NULL == mgr) {
      UPLL_LOG_DEBUG("Unable to get FLOWLIST object");
      return UPLL_RC_ERR_GENERIC;
    }

    /** allocate memory for FLOWLIST key_struct */
    result_code = mgr->GetChildConfigKey(okey, NULL);
    if (result_code != UPLL_RC_SUCCESS) {
      UPLL_LOG_DEBUG("Memory allocation failed for FLOWLIST key struct - %d",
                     result_code);
      return result_code;
    }

    /** fill key_flowlist_t from val_flowfilter_entry*/
    key_flowlist_t *key_flowlist = static_cast<key_flowlist_t*>(
        okey->get_key());
    uuu::upll_strncpy(key_flowlist->flowlist_name,
                      val_flowfilter_entry->flowlist_name,
                      kMaxLenFlowListName+1);

    UPLL_LOG_TRACE("Flowlist name in val_flowfilter_entry %s",
                   key_flowlist->flowlist_name);

    /* Check flowlist_name exists in table*/
    result_code = mgr->UpdateConfigDB(okey, req->datatype,
                                      UNC_OP_READ, dmi, MAINTBL);

    if (UPLL_RC_ERR_INSTANCE_EXISTS != result_code) {
      UPLL_LOG_DEBUG("Flowlist name in val_flowfilter_entry does not exists"
                     "in FLOWLIST table");
      delete okey;
      okey = NULL;
      return UPLL_RC_ERR_CFG_SEMANTIC;
    } else {
      result_code = UPLL_RC_SUCCESS;
    }

    delete okey;
    okey = NULL;
  }
  if (val_flowfilter_entry->valid[UPLL_IDX_NWM_NAME_FFE]
      == UNC_VF_VALID) {
    //  validate nwm_name in KT_VBR_NWMONITOR table
    mgr = reinterpret_cast<MoMgrImpl *>
        (const_cast<MoManager *>(GetMoManager(UNC_KT_VBR_NWMONITOR)));

    if (NULL == mgr) {
      UPLL_LOG_DEBUG("Unable to get KT_VBR_NWMONITOR object");
      return UPLL_RC_ERR_GENERIC;
    }

    /** allocate memory for key_nwm key_struct */
    result_code = mgr->GetChildConfigKey(okey, NULL);
    if (result_code != UPLL_RC_SUCCESS) {
      UPLL_LOG_DEBUG("Memory allocation failed for key_nwm struct - %d",
                     result_code);
      return result_code;
    }

    /** fill key_nwm from key/val VRTIF_FLOWFILTER_ENTRY structs*/
    key_nwm_t *key_nwm = static_cast<key_nwm_t*>(
        okey->get_key());

    uuu::upll_strncpy(key_nwm->nwmonitor_name,
                      val_flowfilter_entry->nwm_name,
                      kMaxLenVnodeName+1);

    uuu::upll_strncpy(key_nwm->vbr_key.vtn_key.vtn_name,
                      key_vrtif_ffe->
                      flowfilter_key.if_key.vrt_key.vtn_key.vtn_name,
                      kMaxLenVtnName+1);

    /* Check nwm_name exists in table*/
    DbSubOp dbop = {kOpReadSingle, kOpMatchNone, kOpInOutNone};
    result_code = mgr->ReadConfigDB(okey, req->datatype, UNC_OP_READ,
                                    dbop, dmi, MAINTBL);

    if (UPLL_RC_SUCCESS != result_code) {
      UPLL_LOG_DEBUG("NWM name in val_flowfilter_entry does not exists"
                     "in KT_VBR_NWMONITOR table");
      delete okey;
      okey = NULL;
      return UPLL_RC_ERR_CFG_SEMANTIC;
    } else {
      result_code = UPLL_RC_SUCCESS;
    }

    delete okey;
    okey = NULL;
  }  //  nwm_name is valid
  UPLL_LOG_DEBUG("ValidateAttribute Successfull.");
  return result_code;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::AllocVal(ConfigVal *&ck_val,
                                              upll_keytype_datatype_t dt_type,
                                              MoMgrTables tbl) {
  UPLL_FUNC_TRACE;
  void *val;  //    *ck_nxtval;
  if (ck_val != NULL) {
    UPLL_LOG_DEBUG("ck_val Consist the Value");
    return UPLL_RC_ERR_GENERIC;
  }
  if (tbl == MAINTBL) {
    val  = reinterpret_cast <void *>(
        ConfigKeyVal::Malloc(sizeof(val_flowfilter_entry_t)));
    ck_val = new ConfigVal(IpctSt::kIpcStValFlowfilterEntry, val);
  } else {
    val = NULL;
    return UPLL_RC_ERR_GENERIC;
  }

  UPLL_LOG_DEBUG("AllocVal Success");
  return UPLL_RC_SUCCESS;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::GetValid(void *val, uint64_t indx,
                                              uint8_t *&valid,
                                              upll_keytype_datatype_t dt_type,
                                              MoMgrTables tbl) {
  UPLL_FUNC_TRACE;
  if (val == NULL) {
    UPLL_LOG_DEBUG("Memory is not Allocated");
    return UPLL_RC_ERR_GENERIC;
  }

  if (tbl == MAINTBL) {
    switch (indx) {
      case uudst::vrt_if_flowfilter_entry::kDbiFlowlistName:
        valid = &(reinterpret_cast<val_flowfilter_entry *>(val))->\
                valid[UPLL_IDX_FLOWLIST_NAME_FFE];
        break;
      case uudst::vrt_if_flowfilter_entry::kDbiAction:
        valid = &(reinterpret_cast<val_flowfilter_entry *>(val))->\
                valid[UPLL_IDX_ACTION_FFE];
        break;
      case uudst::vrt_if_flowfilter_entry::kDbiRedirectNode:
        valid = &(reinterpret_cast<val_flowfilter_entry *>(val))->\
                valid[UPLL_IDX_REDIRECT_NODE_FFE];
        break;
      case uudst::vrt_if_flowfilter_entry::kDbiRedirectPort:
        valid = &(reinterpret_cast<val_flowfilter_entry *>(val))->\
                valid[UPLL_IDX_REDIRECT_PORT_FFE];
        break;
      case uudst::vrt_if_flowfilter_entry::kDbiModifyDstMac:
        valid = &(reinterpret_cast<val_flowfilter_entry *>(val))->\
                valid[UPLL_IDX_MODIFY_DST_MAC_FFE];
        break;
      case uudst::vrt_if_flowfilter_entry::kDbiModifySrcMac:
        valid = &(reinterpret_cast<val_flowfilter_entry *>(val))->\
                valid[UPLL_IDX_MODIFY_SRC_MAC_FFE];
        break;
      case uudst::vrt_if_flowfilter_entry::kDbiNwmName:
        valid = &(reinterpret_cast<val_flowfilter_entry *>(val)->\
                  valid[UPLL_IDX_NWM_NAME_FFE]);
        break;
      case uudst::vrt_if_flowfilter_entry::kDbiDscp:
        valid = &(reinterpret_cast<val_flowfilter_entry *>(val))->\
                valid[UPLL_IDX_DSCP_FFE];
        break;
      case uudst::vrt_if_flowfilter_entry::kDbiPriority:
        valid = &(reinterpret_cast<val_flowfilter_entry *>(val))->\
                valid[UPLL_IDX_PRIORITY_FFE];
        break;
      default:
        return UPLL_RC_ERR_GENERIC;
    }
  } else {
    UPLL_LOG_DEBUG("Invalid Tbl");
    return UPLL_RC_ERR_GENERIC;
  }
  UPLL_LOG_DEBUG("GetValidAttributte is Succesfull");
  return UPLL_RC_SUCCESS;
}
upll_rc_t VrtIfFlowFilterEntryMoMgr::ValidateMessage(IpcReqRespHeader *req,
                                                     ConfigKeyVal *key) {
  UPLL_FUNC_TRACE;

  upll_rc_t rt_code = UPLL_RC_ERR_GENERIC;

  if ((NULL == req) || (NULL == key)) {
    UPLL_LOG_DEBUG("ConfigKeyval is NULL");
    return UPLL_RC_ERR_BAD_REQUEST;
  }

  if (UNC_KT_VRTIF_FLOWFILTER_ENTRY != key->get_key_type()) {
    UPLL_LOG_DEBUG(" Invalid keytype(%d)", key->get_key_type());
    return UPLL_RC_ERR_BAD_REQUEST;
  }
  if (key->get_st_num() != IpctSt::kIpcStKeyVrtIfFlowfilterEntry) {
    UPLL_LOG_DEBUG("Invalid key structure received. received struct num - %d",
                   key->get_st_num());
    return UPLL_RC_ERR_BAD_REQUEST;
  }
  if (req->option2 != UNC_OPT2_NONE) {
    UPLL_LOG_DEBUG(" Error: option2 is not NONE");
    return UPLL_RC_ERR_INVALID_OPTION2;
  }
  if ((req->option1 != UNC_OPT1_NORMAL)
     &&(req->option1 != UNC_OPT1_DETAIL)) {
    UPLL_LOG_DEBUG(" Error: option1 is not NORMAL");
    return UPLL_RC_ERR_INVALID_OPTION1;
  }
  if ((req->option1 != UNC_OPT1_NORMAL)
     &&(req->operation == UNC_OP_READ_SIBLING_COUNT)) {
    UPLL_LOG_DEBUG(" Error: option1 is not NORMAL for ReadSiblingCount");
    return UPLL_RC_ERR_INVALID_OPTION1;
  }
  if ((req->option1 == UNC_OPT1_DETAIL) &&
      (req->datatype != UPLL_DT_STATE)) {
    UPLL_LOG_DEBUG(" Invalid Datatype(%d)", req->datatype);
    return UPLL_RC_ERR_NOT_ALLOWED_FOR_THIS_DT;
  }
  if ((req->datatype == UPLL_DT_IMPORT) && (
          req->operation == UNC_OP_READ ||
          req->operation == UNC_OP_READ_SIBLING ||
          req->operation == UNC_OP_READ_SIBLING_BEGIN ||
          req->operation == UNC_OP_READ_NEXT ||
          req->operation == UNC_OP_READ_BULK ||
          req->operation == UNC_OP_READ_SIBLING_COUNT)) {
    return UPLL_RC_ERR_NOT_ALLOWED_FOR_THIS_DT;
  }


  /** Read key structure */
  key_vrt_if_flowfilter_entry_t *key_vrt_if_flowfilter_entry =
      reinterpret_cast<key_vrt_if_flowfilter_entry_t *>(key->get_key());

  /** Validate key structure */
  if (NULL == key_vrt_if_flowfilter_entry) {
    UPLL_LOG_DEBUG("KT_VRTIF_FLOWFILTER_ENTRY Key structure is empty!!");
    return UPLL_RC_ERR_BAD_REQUEST;
  }

  rt_code = ValidateVrtIfFlowfilterEntryKey(key_vrt_if_flowfilter_entry,
                                            req->operation);

  if (UPLL_RC_SUCCESS != rt_code) {
    UPLL_LOG_DEBUG(" key_vrtif_flowfilter syntax validation failed :"
                   "Err Code - %d",
                   rt_code);
    return rt_code;
  }

  /* validate value structure*/
  if (!key->get_cfg_val()) {
    if ((req->operation == UNC_OP_UPDATE) ||
        (req->operation == UNC_OP_CREATE)) {
      UPLL_LOG_DEBUG("val structure is mandatory");
      return UPLL_RC_ERR_BAD_REQUEST;
    } else {
      UPLL_LOG_TRACE("val structure is optional");
      return UPLL_RC_SUCCESS;
    }
  }

  if (key->get_cfg_val()->get_st_num() !=
      IpctSt::kIpcStValFlowfilterEntry) {
    UPLL_LOG_DEBUG("Invalid val structure received. struct num - %d",
                   (key->get_cfg_val())->get_st_num());
    return UPLL_RC_ERR_BAD_REQUEST;
  }

  val_flowfilter_entry_t *val_flowfilter_entry =
      reinterpret_cast<val_flowfilter_entry_t *>(
          key->get_cfg_val()->get_val());

  if (NULL == val_flowfilter_entry) {
    UPLL_LOG_DEBUG("val_flowfilter_entry structure is null");
    return UPLL_RC_ERR_BAD_REQUEST;
  }

  //  UpdateMo invokes val structure validate function
  //  as UPDATE operation requires dmi
  if (req->operation == UNC_OP_UPDATE)
    return UPLL_RC_SUCCESS;

  return VbrFlowFilterEntryMoMgr::ValidateFlowfilterEntryValue(
      val_flowfilter_entry, req->operation);
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::ValidateVrtIfValStruct(
    IpcReqRespHeader *req,
    ConfigKeyVal *key) {

  val_flowfilter_entry_t *val_flowfilter_entry =
      reinterpret_cast<val_flowfilter_entry_t *>(
          key->get_cfg_val()->get_val());
  /** validate val_flowfilter_entry value structure */
  return VbrFlowFilterEntryMoMgr::ValidateFlowfilterEntryValue(
      val_flowfilter_entry, req->operation);
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::ValidateVrtIfFlowfilterEntryKey(
    key_vrt_if_flowfilter_entry_t* key_vrt_if_flowfilter_entry,
    unc_keytype_operation_t operation) {

  UPLL_FUNC_TRACE;
  upll_rc_t rt_code = UPLL_RC_ERR_GENERIC;

  VrtIfMoMgr *mgrvrtif = reinterpret_cast<VrtIfMoMgr *>(
      const_cast<MoManager*>(GetMoManager(UNC_KT_VRT_IF)));

  if (NULL == mgrvrtif) {
    UPLL_LOG_DEBUG("unable to get VtnMoMgr object to validate key_vtn");
    return UPLL_RC_ERR_GENERIC;
  }

  rt_code = mgrvrtif->ValidateVrtIfKey(
      &(key_vrt_if_flowfilter_entry->flowfilter_key.if_key));

  if (UPLL_RC_SUCCESS != rt_code) {
    UPLL_LOG_DEBUG(" Vrtif_key syntax validation failed :Err Code - %d",
                   rt_code);

    return rt_code;
  }


  /** validate direction */
  if (!ValidateNumericRange(
          key_vrt_if_flowfilter_entry->flowfilter_key.direction,
          (uint8_t) UPLL_FLOWFILTER_DIR_IN,
          (uint8_t) UPLL_FLOWFILTER_DIR_OUT,
          true, true)) {
    UPLL_LOG_DEBUG("direction syntax validation failed ");
    return UPLL_RC_ERR_CFG_SYNTAX;
  }

  if ((operation != UNC_OP_READ_SIBLING_COUNT) &&
      (operation != UNC_OP_READ_SIBLING_BEGIN)) {
    /** validate Sequence number */
    if (!ValidateNumericRange(key_vrt_if_flowfilter_entry->sequence_num,
                              kMinFlowFilterSeqNum, kMaxFlowFilterSeqNum, true,
                              true)) {
      UPLL_LOG_DEBUG("Sequence number validation failed ");
      return UPLL_RC_ERR_CFG_SYNTAX;
    }
  } else {
    key_vrt_if_flowfilter_entry->sequence_num = 0;
  }
  return UPLL_RC_SUCCESS;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::ValidateCapability(
    IpcReqRespHeader *req,
    ConfigKeyVal *ikey,
    const char* ctrlr_name) {
  UPLL_FUNC_TRACE;

  upll_rc_t rt_code = UPLL_RC_ERR_GENERIC;

  if ((NULL == req) || (NULL == ikey)) {
    UPLL_LOG_DEBUG("IpcReqRespHeader/ConfigKeyval is NULL");
    return rt_code;
  }

  if (!ctrlr_name)
    ctrlr_name = static_cast<char *>(ikey->get_user_data());

  UPLL_LOG_TRACE("dt_type   : (%d)"
                 "operation : (%d)", req->datatype, req->operation);

  bool ret_code = false;
  uint32_t instance_count = 0;
  const uint8_t *attrs = NULL;
  uint32_t max_attrs = 0;

  switch (req->operation) {
    case UNC_OP_CREATE: {
      UPLL_LOG_TRACE("Calling GetCreateCapability Operation  %d ",
                     req->operation);
      ret_code = GetCreateCapability(ctrlr_name, ikey->get_key_type(),
                                     &instance_count, &max_attrs, &attrs);
      break;
    }
    case UNC_OP_UPDATE: {
      UPLL_LOG_TRACE("Calling GetUpdateCapability Operation  %d ",
                     req->operation);
      ret_code = GetUpdateCapability(ctrlr_name, ikey->get_key_type(),
                                     &max_attrs, &attrs);
      break;
    }
    default: {
      if (req->datatype == UPLL_DT_STATE) {
        UPLL_LOG_TRACE("Calling GetStateCapability Operation  %d ",
                       req->operation);
        ret_code = GetStateCapability(ctrlr_name, ikey->get_key_type(),
                                      &max_attrs, &attrs);
      } else {
        UPLL_LOG_TRACE("Calling GetReadCapability Operation  %d ",
                       req->operation);
        ret_code = GetReadCapability(ctrlr_name, ikey->get_key_type(),
                                     &max_attrs, &attrs);
      }
      break;
    }
  }

  if (!ret_code) {
    UPLL_LOG_DEBUG("keytype(%d) is not supported by controller(%s)",
                   ikey->get_key_type(), ctrlr_name);
    return UPLL_RC_ERR_NOT_SUPPORTED_BY_CTRLR;
  }
  val_flowfilter_entry_t *val_flowfilter_entry =
      reinterpret_cast<val_flowfilter_entry_t *>(GetVal(ikey));
  if (max_attrs > 0) {
    return VbrFlowFilterEntryMoMgr::ValFlowFilterEntryAttributeSupportCheck(
        val_flowfilter_entry, attrs);
  } else {
    UPLL_LOG_DEBUG("Attribute list is empty for operation %d", req->operation);
    return UPLL_RC_ERR_NOT_SUPPORTED_BY_CTRLR;
  }
}
bool VrtIfFlowFilterEntryMoMgr::GetRenameKeyBindInfo(unc_key_type_t key_type,
                                                     BindInfo *&binfo,
                                                     int &nattr,
                                                     MoMgrTables tbl ) {
  /* Main Table only update */
  if (MAINTBL == tbl) {
    nattr = sizeof(vrt_if_flowfilter_entry_maintbl_bind_info)/
        sizeof(vrt_if_flowfilter_entry_maintbl_bind_info[0]);
    binfo = vrt_if_flowfilter_entry_maintbl_bind_info;
  }

  UPLL_LOG_DEBUG("Successful Completeion");
  return PFC_TRUE;
}
upll_rc_t VrtIfFlowFilterEntryMoMgr::CopyToConfigKey(ConfigKeyVal *&okey,
                                                     ConfigKeyVal *ikey) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;

  if (!ikey || !(ikey->get_key())) {
    UPLL_LOG_DEBUG("Input Key Not Valid");
    return UPLL_RC_ERR_GENERIC;
  }

  key_rename_vnode_info *key_rename = reinterpret_cast<key_rename_vnode_info *>
      (ikey->get_key());
  key_vrt_if_flowfilter_entry_t *key_vrt_if =
      reinterpret_cast<key_vrt_if_flowfilter_entry_t*>
      (ConfigKeyVal::Malloc(sizeof(key_vrt_if_flowfilter_entry_t)));
  //  if (UNC_KT_VRTIF_FLOWFILTER_ENTRY  == ikey->get_key_type()) {
  if (!strlen(reinterpret_cast<char *>(key_rename->old_unc_vtn_name))) {
    UPLL_LOG_DEBUG("String Length not Valid to Perform the Operation");
    free(key_vrt_if);
    return UPLL_RC_ERR_GENERIC;
  }

  uuu::upll_strncpy(key_vrt_if->flowfilter_key.if_key.vrt_key.vtn_key.vtn_name,
                    key_rename->old_unc_vtn_name,
                    (kMaxLenVtnName + 1));

  if (UNC_KT_VROUTER == ikey->get_key_type()) {
    if (!strlen(reinterpret_cast<char *>(key_rename->old_unc_vnode_name))) {
      UPLL_LOG_DEBUG("old_unc_vnode_name NULL");
      free(key_vrt_if);
      return UPLL_RC_ERR_GENERIC;
    }
    uuu::upll_strncpy(key_vrt_if->flowfilter_key.if_key.vrt_key.vrouter_name,
                      key_rename->old_unc_vnode_name, (kMaxLenVnodeName + 1));
  } else {
    if (!strlen(reinterpret_cast<char *>(key_rename->new_unc_vnode_name))) {
      UPLL_LOG_DEBUG("new_unc_vnode_name NULL");
      free(key_vrt_if);
      return UPLL_RC_ERR_GENERIC;
    }

    uuu::upll_strncpy(key_vrt_if->flowfilter_key.if_key.vrt_key.vrouter_name,
                      key_rename->new_unc_vnode_name, (kMaxLenVnodeName + 1));
  }
  key_vrt_if->flowfilter_key.direction = 0xFE;

  okey = new ConfigKeyVal(UNC_KT_VRTIF_FLOWFILTER_ENTRY, IpctSt::
                          kIpcStKeyVrtIfFlowfilterEntry, key_vrt_if, NULL);

  if (!okey) {
    free(key_vrt_if);
    return UPLL_RC_ERR_GENERIC;
  }
  return result_code;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::UpdateVnodeVal(
    ConfigKeyVal *ikey,
    DalDmlIntf *dmi,
    upll_keytype_datatype_t data_type,
    bool &no_rename) {
  UPLL_FUNC_TRACE;
  ConfigKeyVal *okey = NULL;
  ConfigKeyVal *kval = NULL;
  controller_domain ctrlr_dom;
  ctrlr_dom.ctrlr = NULL;
  ctrlr_dom.domain = NULL;

  uint8_t rename = 0;
  upll_rc_t result_code = UPLL_RC_SUCCESS;

  key_rename_vnode_info_t *key_rename =
      reinterpret_cast<key_rename_vnode_info_t *>(ikey->get_key());

  // copy the olf flowlist name to val_flowfilter_entry
  val_flowfilter_entry_t *val_ff_entry =
      reinterpret_cast<val_flowfilter_entry_t *>
      (ConfigKeyVal::Malloc(sizeof(val_flowfilter_entry_t)));

  if (!val_ff_entry) return UPLL_RC_ERR_GENERIC;

  if (ikey->get_key_type() == UNC_KT_FLOWLIST) {
    if (!strlen(reinterpret_cast<char *>(key_rename->old_flowlist_name))) {
      if (val_ff_entry) free(val_ff_entry);
      return UPLL_RC_ERR_GENERIC;
    }

    uuu::upll_strncpy(val_ff_entry->flowlist_name,
                      key_rename->old_flowlist_name,
                      (kMaxLenFlowListName + 1));
    val_ff_entry->valid[UPLL_IDX_FLOWLIST_NAME_FFE] = UNC_VF_VALID;
    UPLL_LOG_DEBUG("valid and flowlist name (%d) (%s)",
                   val_ff_entry->valid[UPLL_IDX_FLOWLIST_NAME_FFE],
                   val_ff_entry->flowlist_name);
  } else if ((ikey->get_key_type() == UNC_KT_VBRIDGE) ||
             (ikey->get_key_type() == UNC_KT_VROUTER)) {
    if (!strlen(reinterpret_cast<char *>(key_rename->old_unc_vnode_name))) {
      UPLL_LOG_DEBUG("key_rename->old_unc_vnode_name NULL");
      if (val_ff_entry) free(val_ff_entry);
      return UPLL_RC_ERR_GENERIC;
    }
    uuu::upll_strncpy(val_ff_entry->redirect_node,
                      key_rename->old_unc_vnode_name,
                      sizeof(val_ff_entry->redirect_node));
    val_ff_entry->valid[UPLL_IDX_REDIRECT_NODE_FFE] = UNC_VF_VALID;
    UPLL_LOG_DEBUG("valid and vbridge name (%d) (%s)",
                   val_ff_entry->valid[UPLL_IDX_REDIRECT_NODE_FFE],
                   val_ff_entry->redirect_node);
  }

  result_code = GetChildConfigKey(okey, NULL);
  if (UPLL_RC_SUCCESS != result_code) {
    UPLL_LOG_DEBUG("CopyToConfigKey okey  NULL");
    free(val_ff_entry);
    return result_code;
  }
  if (!okey) {
    free(val_ff_entry);
    return UPLL_RC_ERR_GENERIC;
  }

  okey->SetCfgVal(new ConfigVal(IpctSt::kIpcStValFlowfilterEntry,
                                val_ff_entry));

  DbSubOp dbop = { kOpReadMultiple,
    kOpMatchNone,
    kOpInOutCtrlr|kOpInOutDomain|kOpInOutFlag };

  //  Read the record of key structure and old flowlist name in maintbl
  result_code = ReadConfigDB(okey, data_type, UNC_OP_READ, dbop, dmi,
                             MAINTBL);
  if (UPLL_RC_SUCCESS != result_code) {
    UPLL_LOG_DEBUG(" ReadConfigDB failed (%d)", result_code);
    DELETE_IF_NOT_NULL(okey);
    return result_code;
  }
  while (okey != NULL) {
    result_code = GetChildConfigKey(kval, okey);
    if (UPLL_RC_SUCCESS != result_code) {
      UPLL_LOG_DEBUG("GetChildConfigKey kval NULL");
      DELETE_IF_NOT_NULL(okey);
      return result_code;
    }
    if (!kval) {
      return UPLL_RC_ERR_GENERIC;
    }
    //  Copy the new flowlist name in val_flowfilter_entry
    val_flowfilter_entry_t *val_ff_entry_new = reinterpret_cast
        <val_flowfilter_entry_t *>
        (ConfigKeyVal::Malloc(sizeof(val_flowfilter_entry_t)));
    if (!val_ff_entry_new) return UPLL_RC_ERR_GENERIC;

    if (ikey->get_key_type() == UNC_KT_FLOWLIST) {
      //  New Name NuLL CHECK
      if (!strlen(reinterpret_cast<char *>(key_rename->new_flowlist_name))) {
        if (val_ff_entry_new) free(val_ff_entry_new);
        UPLL_LOG_DEBUG("new_flowlist_name NULL");
        DELETE_IF_NOT_NULL(kval);
        DELETE_IF_NOT_NULL(okey);
        return UPLL_RC_ERR_GENERIC;
      }

      //  Copy the new flowlist_name into val_flowfilter_entry
      uuu::upll_strncpy(val_ff_entry_new->flowlist_name,
                        key_rename->new_flowlist_name,
                        (kMaxLenFlowListName + 1));
      val_ff_entry_new->valid[UPLL_IDX_FLOWLIST_NAME_FFE] = UNC_VF_VALID;
      UPLL_LOG_DEBUG("flowlist name and valid (%d) (%s)",
                     val_ff_entry_new->valid[UPLL_IDX_FLOWLIST_NAME_FFE],
                     val_ff_entry_new->flowlist_name);
    } else if ((ikey->get_key_type() == UNC_KT_VBRIDGE) ||
               (ikey->get_key_type() == UNC_KT_VROUTER)) {
      //  New Name NuLL CHECK
      if (!strlen(reinterpret_cast<char *>(key_rename->new_unc_vnode_name))) {
        UPLL_LOG_DEBUG("new_unc_vnode_name NULL");
        if (val_ff_entry_new) free(val_ff_entry_new);
        DELETE_IF_NOT_NULL(kval);
        DELETE_IF_NOT_NULL(okey);
        return UPLL_RC_ERR_GENERIC;
      }
      //  Copy the new vbridge name into val_flowfilter_entry
      uuu::upll_strncpy(val_ff_entry_new->redirect_node,
                        key_rename->new_unc_vnode_name,
                        sizeof(val_ff_entry_new->redirect_node));
      val_ff_entry_new->valid[UPLL_IDX_REDIRECT_NODE_FFE] = UNC_VF_VALID;
      UPLL_LOG_DEBUG("vbridge name and valid (%d) (%s)",
                     val_ff_entry_new->valid[UPLL_IDX_FLOWLIST_NAME_FFE],
                     val_ff_entry_new->redirect_node);
    }
    ConfigVal *cval1 = new ConfigVal(IpctSt::kIpcStValFlowfilterEntry,
                                     val_ff_entry_new);

    kval->SetCfgVal(cval1);
    memset(&ctrlr_dom, 0, sizeof(controller_domain));
    result_code = GetControllerDomainID(okey, UPLL_DT_IMPORT, dmi);

    if (result_code != UPLL_RC_SUCCESS) {
      UPLL_LOG_DEBUG("Failed to Get the Controller Domain details, err:%d",
                     result_code);
    }
    GET_USER_DATA_CTRLR_DOMAIN(okey, ctrlr_dom);
    GET_USER_DATA_FLAGS(okey, rename);
    if (ikey->get_key_type() == UNC_KT_FLOWLIST) {
      if (!no_rename)
        rename = rename | FLOW_RENAME;
      else
        rename = rename & NO_FLOWLIST_RENAME;
    } else if (ikey->get_key_type() == UNC_KT_VROUTER) {
      if (!no_rename)
        rename = rename | VRT_RENAME_FLAG;
      else
        rename = rename & NO_VRT_RENAME_FLAG;
    }

    SET_USER_DATA_FLAGS(kval, rename);
    SET_USER_DATA_CTRLR_DOMAIN(kval, ctrlr_dom);

    // Update the new flowlist name in MAINTBL
    result_code = UpdateConfigDB(kval, data_type, UNC_OP_UPDATE, dmi,
                                 MAINTBL);
    if (UPLL_RC_SUCCESS != result_code) {
      UPLL_LOG_DEBUG("Create record Err in vbrflowfilterentrytbl"
                     "CANDIDATE DB(%d)",
                     result_code);
      DELETE_IF_NOT_NULL(kval);
      DELETE_IF_NOT_NULL(okey);
      return result_code;
    }
    DELETE_IF_NOT_NULL(kval);
    okey = okey->get_next_cfg_key_val();
  }
  DELETE_IF_NOT_NULL(okey);
  UPLL_LOG_DEBUG("UpdateVnodeVal result_code (%d)", result_code);
  return result_code;
}
bool  VrtIfFlowFilterEntryMoMgr::CompareValidValue(void *&val1,
                                                   void *val2,
                                                   bool copy_to_running) {
  UPLL_FUNC_TRACE;
  bool attr = true;
  val_flowfilter_entry_t *val_ff_entry1 =
      reinterpret_cast<val_flowfilter_entry_t *>(val1);
  val_flowfilter_entry_t *val_ff_entry2 =
      reinterpret_cast<val_flowfilter_entry_t *>(val2);
  for ( unsigned int loop = 0; loop < sizeof(val_ff_entry1->valid); ++loop ) {
    if (UNC_VF_INVALID == val_ff_entry1->valid[loop] &&
        UNC_VF_VALID == val_ff_entry2->valid[loop])
      val_ff_entry1->valid[loop] = UNC_VF_VALID_NO_VALUE;
  }
  if (val_ff_entry1->valid[UPLL_IDX_FLOWLIST_NAME_FFE] == UNC_VF_VALID &&
      val_ff_entry2->valid[UPLL_IDX_FLOWLIST_NAME_FFE] == UNC_VF_VALID) {
    if (!strcmp(reinterpret_cast<char *>(val_ff_entry1->flowlist_name),
                reinterpret_cast<char *>(val_ff_entry2->flowlist_name)))
      val_ff_entry1->valid[UPLL_IDX_FLOWLIST_NAME_FFE] = UNC_VF_INVALID;
  }
  if (val_ff_entry1->valid[UPLL_IDX_ACTION_FFE] == UNC_VF_VALID &&
      val_ff_entry2->valid[UPLL_IDX_ACTION_FFE] == UNC_VF_VALID) {
    if (val_ff_entry1->action == val_ff_entry2->action)
      val_ff_entry1->valid[UPLL_IDX_ACTION_FFE] = UNC_VF_INVALID;
  }
  if (val_ff_entry1->valid[UPLL_IDX_REDIRECT_NODE_FFE] == UNC_VF_VALID &&
      val_ff_entry2->valid[UPLL_IDX_REDIRECT_NODE_FFE] == UNC_VF_VALID) {
    if (!strcmp(reinterpret_cast<char *>(val_ff_entry1->redirect_node),
                reinterpret_cast<char *>(val_ff_entry2->redirect_node)))
      val_ff_entry1->valid[UPLL_IDX_REDIRECT_NODE_FFE] =
          (copy_to_running)?UNC_VF_INVALID:UNC_VF_VALUE_NOT_MODIFIED;
  }
  if (val_ff_entry1->valid[UPLL_IDX_REDIRECT_PORT_FFE] == UNC_VF_VALID &&
      val_ff_entry2->valid[UPLL_IDX_REDIRECT_PORT_FFE] == UNC_VF_VALID) {
    if (!strcmp(reinterpret_cast<char *>(val_ff_entry1->redirect_port),
                reinterpret_cast<char *>(val_ff_entry2->redirect_port)))
      val_ff_entry1->valid[UPLL_IDX_REDIRECT_PORT_FFE] =
          (copy_to_running)?UNC_VF_INVALID:UNC_VF_VALUE_NOT_MODIFIED;
  }
  if (val_ff_entry1->valid[UPLL_IDX_MODIFY_DST_MAC_FFE] == UNC_VF_VALID &&
      val_ff_entry2->valid[UPLL_IDX_MODIFY_DST_MAC_FFE] == UNC_VF_VALID) {
    if (!memcmp(reinterpret_cast<char*>(val_ff_entry1->modify_dstmac),
                reinterpret_cast<char *>(val_ff_entry2->modify_dstmac),
                sizeof(val_ff_entry2->modify_dstmac))) {
      val_ff_entry1->valid[UPLL_IDX_MODIFY_DST_MAC_FFE] =
          (copy_to_running)?UNC_VF_INVALID:UNC_VF_VALUE_NOT_MODIFIED;
    }
  }
  if (val_ff_entry1->valid[UPLL_IDX_MODIFY_SRC_MAC_FFE] == UNC_VF_VALID &&
      val_ff_entry2->valid[UPLL_IDX_MODIFY_SRC_MAC_FFE] == UNC_VF_VALID) {
    if (!memcmp(reinterpret_cast<char*>(val_ff_entry1->modify_srcmac),
                reinterpret_cast<char*>(val_ff_entry2->modify_srcmac),
                sizeof(val_ff_entry2->modify_srcmac))) {
      val_ff_entry1->valid[UPLL_IDX_MODIFY_SRC_MAC_FFE] =
          (copy_to_running)?UNC_VF_INVALID:UNC_VF_VALUE_NOT_MODIFIED;
    }
  }
  if (val_ff_entry1->valid[UPLL_IDX_NWM_NAME_FFE] == UNC_VF_VALID &&
      val_ff_entry2->valid[UPLL_IDX_NWM_NAME_FFE] == UNC_VF_VALID) {
    if (!strcmp(reinterpret_cast<char *>(val_ff_entry1->nwm_name),
                reinterpret_cast<char *>(val_ff_entry2->nwm_name)))
      val_ff_entry1->valid[UPLL_IDX_NWM_NAME_FFE] = UNC_VF_INVALID;
  }
  if (val_ff_entry1->valid[UPLL_IDX_DSCP_FFE] == UNC_VF_VALID &&
      val_ff_entry2->valid[UPLL_IDX_DSCP_FFE] == UNC_VF_VALID) {
    if (val_ff_entry1->dscp == val_ff_entry2->dscp)
      val_ff_entry1->valid[UPLL_IDX_DSCP_FFE] = UNC_VF_INVALID;
  }
  if (val_ff_entry1->valid[UPLL_IDX_PRIORITY_FFE] == UNC_VF_VALID &&
      val_ff_entry2->valid[UPLL_IDX_PRIORITY_FFE] == UNC_VF_VALID) {
    if (val_ff_entry1->priority == val_ff_entry2->priority)
      val_ff_entry1->valid[UPLL_IDX_PRIORITY_FFE] = UNC_VF_INVALID;
  }
  UPLL_LOG_DEBUG("CompareValidValue :: Success");
  for (unsigned int loop = 0;
       loop < sizeof(val_ff_entry1->valid)/ sizeof(uint8_t); ++loop) {
    if ((UNC_VF_VALID == (uint8_t) val_ff_entry1->valid[loop]) ||
        (UNC_VF_VALID_NO_VALUE == (uint8_t) val_ff_entry1->valid[loop]))
      attr = false;
  }
  return attr;
}
upll_rc_t VrtIfFlowFilterEntryMoMgr::VerifyRedirectDestination(
    ConfigKeyVal *ikey,
    DalDmlIntf *dmi,
    upll_keytype_datatype_t dt_type) {
  UPLL_FUNC_TRACE;
  MoMgrImpl *mgr = NULL;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  ConfigKeyVal *okey = NULL;
  if (!ikey || !ikey->get_key()) {
    UPLL_LOG_DEBUG("input key is null");
    return UPLL_RC_ERR_GENERIC;
  }

  controller_domain ctrlr_dom;
  memset(&ctrlr_dom, 0, sizeof(controller_domain));
  key_vrt_if_flowfilter_entry_t *key_vrtif_ffe =
      reinterpret_cast<key_vrt_if_flowfilter_entry_t *>(ikey->get_key());

  /* read val_vtn_flowfilter_entry from ikey*/
  val_flowfilter_entry_t *val_flowfilter_entry =
      static_cast<val_flowfilter_entry_t *>(
          ikey->get_cfg_val()->get_val());
  //  Symentic Validation for redirect destination
  if ((val_flowfilter_entry->valid[UPLL_IDX_REDIRECT_NODE_FFE] ==
       UNC_VF_VALID) &&
      (val_flowfilter_entry->valid[UPLL_IDX_REDIRECT_PORT_FFE] ==
       UNC_VF_VALID)) {
    DbSubOp dbop_up = { kOpReadExist, kOpMatchCtrlr|kOpMatchDomain,
      kOpInOutNone };
    result_code = GetControllerDomainID(ikey, dt_type, dmi);
    if (result_code != UPLL_RC_SUCCESS) {
      UPLL_LOG_DEBUG("Failed to Get the Controller Domain details, err:%d",
                     result_code);
    }

    GET_USER_DATA_CTRLR_DOMAIN(ikey, ctrlr_dom);

    UPLL_LOG_DEBUG("ctrlrid %s, domainid %s",
                   ctrlr_dom.ctrlr, ctrlr_dom.domain);
    //  Verify whether the vtnnode and interface are exists in DB
    //  1. Check for the vbridge Node
    mgr = reinterpret_cast<MoMgrImpl *>
        (const_cast<MoManager *>(GetMoManager(UNC_KT_VBR_IF)));
    if (NULL == mgr) {
      UPLL_LOG_DEBUG("Unable to get VBRIDGE Interface object");
      return UPLL_RC_ERR_GENERIC;
    }
    result_code = mgr->GetChildConfigKey(okey, NULL);
    if (result_code != UPLL_RC_SUCCESS) {
      UPLL_LOG_DEBUG("Memory allocation failed for VBRIDGE key struct - %d",
                     result_code);
      return result_code;
    }
    key_vbr_if_t *vbrif_key = static_cast<key_vbr_if_t*>(
        okey->get_key());
    uuu::upll_strncpy(vbrif_key->vbr_key.vtn_key.vtn_name,
                      key_vrtif_ffe->
                      flowfilter_key.if_key.vrt_key.vtn_key.vtn_name,
                      kMaxLenVtnName + 1);

    uuu::upll_strncpy(vbrif_key->vbr_key.vbridge_name,
                      reinterpret_cast<char *>
                      (val_flowfilter_entry->redirect_node),
                      (kMaxLenVnodeName + 1));
    uuu::upll_strncpy(vbrif_key->if_name,
                      reinterpret_cast<char *>
                      (val_flowfilter_entry->redirect_port),
                      kMaxLenInterfaceName + 1);

    /* Check vtnnode and interface exists in table*/

    SET_USER_DATA_CTRLR_DOMAIN(okey, ctrlr_dom);

    result_code = mgr->UpdateConfigDB(okey, dt_type,
                                      UNC_OP_READ, dmi, &dbop_up, MAINTBL);

    DELETE_IF_NOT_NULL(okey);
    if (UPLL_RC_ERR_INSTANCE_EXISTS == result_code) {
      UPLL_LOG_DEBUG("vtn node/interface in val_flowfilter_entry  exists"
                     "in DB");
    } else if (UPLL_RC_ERR_NO_SUCH_INSTANCE == result_code) {
      //  2. Check for Vrouter Node

      //  Verify whether the vtnnode and interface are exists in DB
      mgr = reinterpret_cast<MoMgrImpl *>
          (const_cast<MoManager *>(GetMoManager(UNC_KT_VRT_IF)));
      if (NULL == mgr) {
        UPLL_LOG_DEBUG("Unable to get VROUTER Interface object");
        return UPLL_RC_ERR_GENERIC;
      }
      result_code = mgr->GetChildConfigKey(okey, NULL);
      if (result_code != UPLL_RC_SUCCESS) {
        UPLL_LOG_DEBUG("Memory allocation failed for VROUTER key struct - %d",
                       result_code);
        return result_code;
      }
      key_vrt_if_t *vrtif_key = static_cast<key_vrt_if_t*>(
          okey->get_key());
      uuu::upll_strncpy(vrtif_key->vrt_key.vtn_key.vtn_name,
                        key_vrtif_ffe->
                        flowfilter_key.if_key.vrt_key.vtn_key.vtn_name,
                        kMaxLenVtnName + 1);

      uuu::upll_strncpy(vrtif_key->vrt_key.vrouter_name,
                        reinterpret_cast<char *>
                        (val_flowfilter_entry->redirect_node),
                        (kMaxLenVnodeName + 1));
      uuu::upll_strncpy(vrtif_key->if_name,
                        reinterpret_cast<char *>
                        (val_flowfilter_entry->redirect_port),
                        kMaxLenInterfaceName + 1);

      UPLL_LOG_DEBUG("ctrlrid %s, domainid %s",
                     ctrlr_dom.ctrlr, ctrlr_dom.domain);
      SET_USER_DATA_CTRLR_DOMAIN(okey, ctrlr_dom);
      /* Check vtnnode and interface exists in table*/
      result_code = mgr->UpdateConfigDB(okey, dt_type,
                                        UNC_OP_READ, dmi, &dbop_up, MAINTBL);
      DELETE_IF_NOT_NULL(okey);
      if (UPLL_RC_ERR_INSTANCE_EXISTS != result_code) {
        UPLL_LOG_DEBUG("vtn node/interface in val struct does not exists"
                       "in DB");
        return UPLL_RC_ERR_CFG_SEMANTIC;
      }
    }
    result_code =
        (result_code == UPLL_RC_ERR_INSTANCE_EXISTS) ? UPLL_RC_SUCCESS :
        result_code;
  }  //  end of Symentic Validation
  return result_code;
}

/*Return result of validation*/
upll_rc_t VrtIfFlowFilterEntryMoMgr::TxVote(unc_key_type_t keytype,
                                            DalDmlIntf *dmi,
                                            ConfigKeyVal **err_ckv) {
  UPLL_FUNC_TRACE;
  ConfigKeyVal *req = NULL, *nreq = NULL;
  DalResultCode db_result;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  unc_keytype_operation_t op[]= { UNC_OP_CREATE, UNC_OP_UPDATE};
  int nop = sizeof(op) / sizeof(op[0]);
  DalCursor *cfg1_cursor = NULL;

  for (int i = 0; i < nop; i++) {
    cfg1_cursor = NULL;
    result_code = DiffConfigDB(UPLL_DT_CANDIDATE, UPLL_DT_RUNNING, op[i], req,
                               nreq, &cfg1_cursor, dmi, MAINTBL);
    if (result_code == UPLL_RC_ERR_NO_SUCH_INSTANCE) {
      result_code = UPLL_RC_SUCCESS;
      UPLL_LOG_DEBUG("No more diff found for operation %d", op[i]);
      DELETE_IF_NOT_NULL(req);
      DELETE_IF_NOT_NULL(nreq);
      continue;
    }
    while (result_code == UPLL_RC_SUCCESS) {
      db_result = dmi->GetNextRecord(cfg1_cursor);
      result_code = DalToUpllResCode(db_result);
      if (result_code == UPLL_RC_ERR_NO_SUCH_INSTANCE) {
        result_code = UPLL_RC_SUCCESS;
        UPLL_LOG_DEBUG("No more diff found for operation %d", op[i]);
        break;
      }
      val_flowfilter_entry_t* val = reinterpret_cast<val_flowfilter_entry_t *>
          (GetVal(req));
      if ((val->valid[UPLL_IDX_REDIRECT_NODE_FFE] ==
           UNC_VF_VALID) &&
          (val->valid[UPLL_IDX_REDIRECT_PORT_FFE] ==
           UNC_VF_VALID)) {
        result_code = VerifyRedirectDestination(req, dmi,
                                                UPLL_DT_CANDIDATE);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("Invalid redirect-destination node/interface");
          DEL_USER_DATA(req);  //  Delete the controller and domain from req
          *err_ckv = req;
          DELETE_IF_NOT_NULL(nreq);
          dmi->CloseCursor(cfg1_cursor, true);
          return result_code;
        }
      }
    }
    dmi->CloseCursor(cfg1_cursor, true);
    if (req) delete req;
    if (nreq) delete nreq;
  }

  return result_code;
}
upll_rc_t VrtIfFlowFilterEntryMoMgr::TxUpdateController(
    unc_key_type_t keytype,
    uint32_t session_id,
    uint32_t config_id,
    uuc::UpdateCtrlrPhase phase,
    set<string> *affected_ctrlr_set,
    DalDmlIntf *dmi,
    ConfigKeyVal **err_ckv)  {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  ConfigKeyVal *req, *nreq = NULL, *ck_main = NULL;
  controller_domain_t ctrlr_dom;
  ctrlr_dom.ctrlr = NULL;
  ctrlr_dom.domain = NULL;
  DalCursor *dal_cursor_handle = NULL;
  IpcResponse ipc_resp;
  uint8_t flag = 0;
  DalResultCode db_result;
  uint8_t db_flag = 0;
  if (affected_ctrlr_set == NULL)
    return UPLL_RC_ERR_GENERIC;

  unc_keytype_operation_t op = (phase == uuc::kUpllUcpCreate)?UNC_OP_CREATE:
      ((phase == uuc::kUpllUcpUpdate)?UNC_OP_UPDATE:
       ((phase == uuc::kUpllUcpDelete)?UNC_OP_DELETE:UNC_OP_INVALID));

  result_code = DiffConfigDB(UPLL_DT_CANDIDATE, UPLL_DT_RUNNING,
                             op, req, nreq, &dal_cursor_handle, dmi, MAINTBL);
  unc_keytype_operation_t op1 = op;

  while (result_code == UPLL_RC_SUCCESS) {
    ck_main = NULL;
    db_result = dmi->GetNextRecord(dal_cursor_handle);
    result_code = DalToUpllResCode(db_result);
    if (result_code != UPLL_RC_SUCCESS)
      break;
    switch (op)   {
      case UNC_OP_CREATE:
      case UNC_OP_UPDATE:
        /* fall through intended */
        /*Restore the original op code for each DB record*/
        op1 = op;
        result_code = DupConfigKeyVal(ck_main, req, MAINTBL);
        if (!ck_main || result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_TRACE("DupConfigKeyVal failed %d", result_code);
          DELETE_IF_NOT_NULL(ck_main);
          return result_code;
        }
        break;
      case UNC_OP_DELETE:
        {
          /*Restore the original op code for each DB record*/
          op1 = op;
          result_code = GetChildConfigKey(ck_main, req);
          if (!ck_main || result_code != UPLL_RC_SUCCESS) {
            UPLL_LOG_TRACE("GetChildConfigKey failed %d", result_code);
            DELETE_IF_NOT_NULL(ck_main);
            return result_code;
          }
          DbSubOp dbop = { kOpReadSingle, kOpMatchNone, kOpInOutCtrlr};
          result_code = ReadConfigDB(ck_main, UPLL_DT_RUNNING, UNC_OP_READ,
                                     dbop, dmi, MAINTBL);
          if (result_code != UPLL_RC_SUCCESS) {
            UPLL_LOG_DEBUG("Returning error %d", result_code);
            return UPLL_RC_ERR_GENERIC;
          }
        }
      default:
        break;
    }
    if (!ck_main) return UPLL_RC_ERR_GENERIC;
    GET_USER_DATA_CTRLR_DOMAIN(ck_main, ctrlr_dom);
    if (ctrlr_dom.ctrlr == NULL) {
      return UPLL_RC_ERR_GENERIC;
    }
    /*
       if ((op == UNC_OP_CREATE) || (op == UNC_OP_UPDATE)) {
       void *main = GetVal(ck_main);
       void *val_nrec = (nreq) ? GetVal(nreq) : NULL;
       FilterAttributes(main, val_nrec, false, op);
       }
       */
    GET_USER_DATA_FLAGS(ck_main, db_flag);
    if (!(SET_FLAG_VLINK & db_flag)) {
      if (op1 != UNC_OP_UPDATE) {
        continue;
      }
      ConfigKeyVal *temp = NULL;
      result_code = GetChildConfigKey(temp, ck_main);
      if (result_code != UPLL_RC_SUCCESS) {
        UPLL_LOG_DEBUG("GetChildConfigKey failed, err %d", result_code);
        return result_code;
      }
      //  SET_USER_DATA_CTRLR_DOMAIN(temp, ctrlr_dom);
      DbSubOp dbop1 = { kOpReadSingle, kOpMatchCtrlr|kOpMatchDomain,
        kOpInOutFlag};
      result_code = ReadConfigDB(temp, UPLL_DT_RUNNING,
                                 UNC_OP_READ, dbop1, dmi, MAINTBL);
      if (result_code != UPLL_RC_SUCCESS) {
        if (result_code != UPLL_RC_ERR_INSTANCE_EXISTS) {
          UPLL_LOG_DEBUG("Unable to read from DB, err: %d", result_code);
          return result_code;
        }
      }
      GET_USER_DATA_FLAGS(temp, flag);
      if (!(SET_FLAG_VLINK & flag)) {
        UPLL_LOG_DEBUG("Vlink flag is not set for VrtIfFlowFilterEntry");
        continue;
      }
      op1 = UNC_OP_DELETE;
    } else {
      if (UNC_OP_UPDATE == op1) {
        ConfigKeyVal *temp = NULL;
        result_code = GetChildConfigKey(temp, ck_main);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_TRACE("GetChildConfigKey failed %d", result_code);
          return result_code;
        }
        DbSubOp dbop = { kOpReadSingle, kOpMatchCtrlr|kOpMatchDomain,
          kOpInOutFlag};
        result_code = ReadConfigDB(temp, UPLL_DT_RUNNING, UNC_OP_READ,
                                   dbop, dmi, MAINTBL);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("Returning error %d", result_code);
          DELETE_IF_NOT_NULL(temp);
          return UPLL_RC_ERR_GENERIC;
        }
        GET_USER_DATA_FLAGS(temp, flag);
        if (!(SET_FLAG_VLINK & flag)) {
          op1 = UNC_OP_CREATE;
        } else {
          void *main = GetVal(ck_main);
          void *val_nrec = (nreq) ? GetVal(nreq) : NULL;
          FilterAttributes(main, val_nrec, false, op);
        }
        DELETE_IF_NOT_NULL(temp);
      }
    }

    pfcdrv_val_flowfilter_entry_t *pfc_val =
        reinterpret_cast<pfcdrv_val_flowfilter_entry_t *>
        (ConfigKeyVal::Malloc(sizeof(pfcdrv_val_flowfilter_entry_t)));

    pfc_val->valid[PFCDRV_IDX_VAL_VBRIF_VEXTIF_FFE] = UNC_VF_VALID;
    pfc_val->val_vbrif_vextif.valid[PFCDRV_IDX_INTERFACE_TYPE] = UNC_VF_VALID;
    pfc_val->val_vbrif_vextif.interface_type = PFCDRV_IF_TYPE_VBRIF;
#if 0
    val_flowfilter_entry_t* val = reinterpret_cast<val_flowfilter_entry_t *>
        (GetVal(ck_main));
    memcpy(&pfc_val->val_ff_entry, val, sizeof(val_flowfilter_entry_t));
#endif
    //  Inserting the controller to Set
    affected_ctrlr_set->insert
        (string(reinterpret_cast<char *>(ctrlr_dom.ctrlr)));
    ConfigKeyVal *temp_ck_main = NULL;
    result_code = DupConfigKeyVal(temp_ck_main, req, MAINTBL);
    if (UPLL_RC_SUCCESS != result_code) {
      UPLL_LOG_DEBUG("DupConfigKeyVal failed %d", result_code);
      DELETE_IF_NOT_NULL(ck_main);
      return result_code;
    }
    upll_keytype_datatype_t dt_type = (op1 == UNC_OP_DELETE)?
        UPLL_DT_RUNNING:UPLL_DT_CANDIDATE;
    result_code = GetRenamedControllerKey(ck_main, dt_type,
                                          dmi, &ctrlr_dom);
    if (result_code != UPLL_RC_SUCCESS)
      break;
    if (UNC_OP_DELETE == op1) {
      pfc_val->valid[PFCDRV_IDX_FLOWFILTER_ENTRY_FFE] = UNC_VF_INVALID;
    } else {
      val_flowfilter_entry_t* val = reinterpret_cast<val_flowfilter_entry_t *>
          (GetVal(ck_main));
      memcpy(&pfc_val->val_ff_entry, val, sizeof(val_flowfilter_entry_t));
      pfc_val->valid[PFCDRV_IDX_FLOWFILTER_ENTRY_FFE] = UNC_VF_VALID;
    }

    UPLL_LOG_DEBUG("Controller : %s; Domain : %s", ctrlr_dom.ctrlr,
                   ctrlr_dom.domain);
    ck_main->SetCfgVal(new ConfigVal(IpctSt::kIpcStPfcdrvValFlowfilterEntry,
                                     pfc_val));
    result_code = SendIpcReq(session_id, config_id, op1, UPLL_DT_CANDIDATE,
                             ck_main, &ctrlr_dom, &ipc_resp);
    if (result_code == UPLL_RC_ERR_CTR_DISCONNECTED) {
      UPLL_LOG_DEBUG(" driver result code - %d", result_code);
      result_code = UPLL_RC_SUCCESS;
    }
    if (result_code != UPLL_RC_SUCCESS) {
      UPLL_LOG_DEBUG("IpcSend failed %d", result_code);
      *err_ckv = temp_ck_main;
      DELETE_IF_NOT_NULL(ipc_resp.ckv_data);
      DELETE_IF_NOT_NULL(ck_main);
      break;
    }
    DELETE_IF_NOT_NULL(ipc_resp.ckv_data);
    DELETE_IF_NOT_NULL(temp_ck_main);
    DELETE_IF_NOT_NULL(ck_main);
  }
  if (dal_cursor_handle) {
    dmi->CloseCursor(dal_cursor_handle, true);
    dal_cursor_handle = NULL;
  }
  if (req)
    delete req;
  if (nreq)
    delete nreq;
  result_code = (result_code == UPLL_RC_ERR_NO_SUCH_INSTANCE) ?
      UPLL_RC_SUCCESS : result_code;

  return result_code;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::SetVlinkPortmapConfiguration(
    ConfigKeyVal *ikey,
    upll_keytype_datatype_t dt_type,
    DalDmlIntf *dmi, InterfacePortMapInfo flag,
    unc_keytype_operation_t oper) {
  upll_rc_t result_code = UPLL_RC_ERR_GENERIC;
  controller_domain ctrlr_dom;
  uint8_t *ctrlr_id = NULL;

  if (NULL == ikey || NULL == ikey->get_key()) {
    return result_code;
  }
  ConfigKeyVal *ckv = NULL;
  memset(&ctrlr_dom, 0, sizeof(controller_domain));
  result_code = GetChildConfigKey(ckv, ikey);
  if (UPLL_RC_SUCCESS != result_code) {
    return result_code;
  }
  DbSubOp dbop = { kOpReadMultiple, kOpMatchNone, kOpInOutFlag };
  result_code = ReadConfigDB(ckv, dt_type ,
                             UNC_OP_READ, dbop, dmi, MAINTBL);

  if (UPLL_RC_ERR_NO_SUCH_INSTANCE == result_code) {
    UPLL_LOG_DEBUG("No Recrods in the Vbr_If_FlowFilter Table");
    DELETE_IF_NOT_NULL(ckv);
    return UPLL_RC_SUCCESS;
  }
  if (UPLL_RC_SUCCESS != result_code) {
    UPLL_LOG_DEBUG("Read ConfigDB failure %d", result_code);
    DELETE_IF_NOT_NULL(ckv);
    return result_code;
  }
  uint8_t  flag_port_map = 0;
  ConfigKeyVal *ckv_first = ckv;
  while (ckv) {
    val_flowfilter_entry_t *flowfilter_val =
        reinterpret_cast<val_flowfilter_entry_t *> (GetVal(ckv));
    flag_port_map = 0;
    GET_USER_DATA_FLAGS(ckv, flag_port_map);
    if (flag & kVlinkConfigured) {
      UPLL_LOG_DEBUG("Vlink Flag");
      flag_port_map |= SET_FLAG_VLINK;
    } else {
      UPLL_LOG_DEBUG("No Vlink Flag");
      flag_port_map &= NO_FLAG_VLINK;
    }
    UPLL_LOG_DEBUG("SET_USER_DATA_FLAGS flag_port_map %d", flag_port_map);
    SET_USER_DATA_FLAGS(ckv, flag_port_map);

    DbSubOp dbop_update = { kOpReadSingle, kOpMatchNone, kOpInOutFlag };
    result_code = UpdateConfigDB(ckv, dt_type, UNC_OP_UPDATE,
                                 dmi, &dbop_update, MAINTBL);
    if (UPLL_RC_SUCCESS != result_code) {
      UPLL_LOG_DEBUG("Failed to update flag in DB , err %d", result_code);
      DELETE_IF_NOT_NULL(ckv);
      return result_code;
    }
    if (flowfilter_val->valid[UPLL_IDX_FLOWLIST_NAME_FFE] == UNC_VF_VALID) {
      unc_keytype_operation_t op = UNC_OP_INVALID;
      FlowListMoMgr *mgr = reinterpret_cast<FlowListMoMgr *>
          (const_cast<MoManager *> (GetMoManager(UNC_KT_FLOWLIST)));
      if (flag_port_map & SET_FLAG_VLINK) {
        op = UNC_OP_CREATE;
      } else  {
        op = UNC_OP_DELETE;
      }
      result_code = GetControllerDomainID(ikey, dt_type, dmi);
      if (result_code != UPLL_RC_SUCCESS) {
        UPLL_LOG_DEBUG("Failed to Get the Controller Domain details, err:%d",
                       result_code);
        DELETE_IF_NOT_NULL(ckv);
        return result_code;
      }
      GET_USER_DATA_CTRLR_DOMAIN(ikey, ctrlr_dom);
      UPLL_LOG_DEBUG("ctrlrid %s, domainid %s",
                     ctrlr_dom.ctrlr, ctrlr_dom.domain);
      ctrlr_id = ctrlr_dom.ctrlr;
      result_code = mgr->AddFlowListToController(
          reinterpret_cast<char*>(flowfilter_val->flowlist_name), dmi,
          reinterpret_cast<char *>(ctrlr_id), dt_type, op);
      if (result_code != UPLL_RC_SUCCESS) {
        UPLL_LOG_DEBUG(" Send delete request to flowlist failed. err code(%d)",
                       result_code);
        DELETE_IF_NOT_NULL(ckv);
        return result_code;
      }
    }

    ckv = ckv->get_next_cfg_key_val();
  }
  DELETE_IF_NOT_NULL(ckv_first);
  return UPLL_RC_SUCCESS;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::RestorePOMInCtrlTbl(
    ConfigKeyVal *ikey,
    upll_keytype_datatype_t dt_type,
    MoMgrTables tbl,
    DalDmlIntf* dmi) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  controller_domain ctrlr_dom;
  FlowListMoMgr *mgr = NULL;
  uint8_t *ctrlr_id = NULL;

  if (!ikey || !(ikey->get_key())) {
    UPLL_LOG_DEBUG("Input Key Not Valid");
    return UPLL_RC_ERR_GENERIC;
  }
  if (tbl != MAINTBL ||
      ikey->get_key_type() != UNC_KT_VRTIF_FLOWFILTER_ENTRY) {
    UPLL_LOG_DEBUG("Ignoring  ktype/Table kt=%d, tbl=%d",
                   ikey->get_key_type(), tbl);
    return result_code;
  }

  memset(&ctrlr_dom, 0, sizeof(controller_domain));
  val_flowfilter_entry_t *flowfilter_val =
      reinterpret_cast<val_flowfilter_entry_t *> (GetVal(ikey));
  if (NULL == flowfilter_val) {
    UPLL_LOG_DEBUG(" Value structure is NULL");
    return UPLL_RC_ERR_GENERIC;
  }
  if (flowfilter_val->valid[UPLL_IDX_FLOWLIST_NAME_FFE] == UNC_VF_VALID) {
    result_code = GetControllerDomainID(ikey, dt_type, dmi);
    if (result_code != UPLL_RC_SUCCESS) {
      UPLL_LOG_DEBUG("Failed to Get the Controller Domain details, err:%d",
                     result_code);
      return result_code;
    }
    GET_USER_DATA_CTRLR_DOMAIN(ikey, ctrlr_dom);
    UPLL_LOG_DEBUG("ctrlrid %s, domainid %s",
                   ctrlr_dom.ctrlr, ctrlr_dom.domain);
    ctrlr_id = ctrlr_dom.ctrlr;

    mgr = reinterpret_cast<FlowListMoMgr *>
        (const_cast<MoManager *> (GetMoManager(UNC_KT_FLOWLIST)));
    result_code = mgr->AddFlowListToController(
        reinterpret_cast<char *>(flowfilter_val->flowlist_name),
        dmi,
        reinterpret_cast<char *>(ctrlr_id),
        dt_type,
        UNC_OP_CREATE);
    if (result_code != UPLL_RC_SUCCESS) {
      UPLL_LOG_DEBUG("Unable to update the FlowList at ctrlr table.Err %d",
                     result_code);
      return result_code;
    }
  }
  return result_code;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::CreateCandidateMo(IpcReqRespHeader *req,
                                                       ConfigKeyVal *ikey,
                                                       DalDmlIntf *dmi,
                                                       bool restore_flag) {
  UPLL_FUNC_TRACE;
  uint8_t *ctrlr_id = NULL;
  controller_domain ctrlr_dom;
  memset(&ctrlr_dom, 0, sizeof(ctrlr_dom));
  if (ikey == NULL || req == NULL) {
    UPLL_LOG_DEBUG(
        "Cannot perform create operation due to insufficient parameters");
    return UPLL_RC_ERR_GENERIC;
  }
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  val_flowfilter_entry_t *val_ff_import = NULL;
  pfcdrv_val_flowfilter_entry_t *pfc_val_import = NULL;

  if (req->datatype == UPLL_DT_IMPORT) {
    UPLL_LOG_DEBUG("Inside %d", req->datatype);
    if (ikey->get_cfg_val() &&
        (ikey->get_cfg_val()->get_st_num() ==
         IpctSt::kIpcStPfcdrvValFlowfilterEntry)) {
      UPLL_LOG_DEBUG("pran:-val struct num (%d)", ikey->
                     get_cfg_val()->get_st_num());
      pfc_val_import = reinterpret_cast<pfcdrv_val_flowfilter_entry_t *>
          (ikey->get_cfg_val()->get_val());
      val_ff_import = reinterpret_cast<val_flowfilter_entry_t *>
          (ConfigKeyVal::Malloc(sizeof(val_flowfilter_entry_t)));
      memcpy(val_ff_import, &pfc_val_import->val_ff_entry,
             sizeof(val_flowfilter_entry_t));
      UPLL_LOG_DEBUG("FLOWLIST name (%s)", val_ff_import->flowlist_name);
      ikey->SetCfgVal(NULL);
      ikey->SetCfgVal(new ConfigVal(IpctSt::kIpcStValFlowfilterEntry,
                                    val_ff_import));
    }
  }
  UPLL_LOG_TRACE("%s vrt_if_ff_entry", ikey->ToStrAll().c_str());
  if (!restore_flag) {
    //  validate syntax and semantics
    result_code = ValidateMessage(req, ikey);
    if (result_code != UPLL_RC_SUCCESS) {
      UPLL_LOG_DEBUG(" ValidateMessage failed ");
      return result_code;
    }
  }

  result_code = ValidateAttribute(ikey, dmi, req);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_DEBUG(" ValidateAttribute failed ");
    return result_code;
  }

  val_flowfilter_entry_t *flowfilter_val =
      reinterpret_cast<val_flowfilter_entry_t *> (GetVal(ikey));
  result_code = GetControllerDomainID(ikey, req->datatype, dmi);
  if (result_code != UPLL_RC_SUCCESS) {
    if (UPLL_RC_ERR_NO_SUCH_INSTANCE == result_code) {
      result_code = UPLL_RC_ERR_PARENT_DOES_NOT_EXIST;
    }
    UPLL_LOG_DEBUG("Failed to Get the Controller Domain details, err:%d",
                   result_code);
    return result_code;
  }
  GET_USER_DATA_CTRLR_DOMAIN(ikey, ctrlr_dom);

  UPLL_LOG_DEBUG("ctrlrid %s, domainid %s",
                 ctrlr_dom.ctrlr, ctrlr_dom.domain);
  ctrlr_id = ctrlr_dom.ctrlr;

  result_code = ValidateCapability(req, ikey,
                                   reinterpret_cast<const char *>(ctrlr_id));
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_DEBUG("validate Capability Failed %d", result_code);
    return result_code;
  }
  if (!restore_flag) {
    if (UPLL_DT_CANDIDATE == req->datatype) {
      result_code = UpdateConfigDB(ikey, UPLL_DT_RUNNING, UNC_OP_READ, dmi,
                                   MAINTBL);
      if (result_code == UPLL_RC_ERR_INSTANCE_EXISTS) {
        UPLL_LOG_DEBUG("Key instance exist");
        if ((ikey)->get_cfg_val()) {
          UPLL_LOG_DEBUG("Read Key with Value struct");
          DbSubOp dbop = { kOpReadSingle, kOpMatchNone,
            kOpInOutFlag | kOpInOutCtrlr | kOpInOutDomain };
          result_code = ReadConfigDB(ikey, UPLL_DT_RUNNING, UNC_OP_READ, dbop,
                                     dmi, MAINTBL);
          if (UPLL_RC_SUCCESS != result_code &&
              UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code) {
            UPLL_LOG_DEBUG("ReadConfigDB Failed %d",  result_code);
          }
          if (UPLL_RC_ERR_NO_SUCH_INSTANCE == result_code)  {
            return UPLL_RC_ERR_CFG_SEMANTIC;
          }
        } else  {
          result_code = UPLL_RC_SUCCESS;
        }
        if (UPLL_RC_SUCCESS == result_code) {
          result_code = RestoreChildren(ikey,
                                        req->datatype,
                                        UPLL_DT_RUNNING,
                                        dmi,
                                        req);
          UPLL_LOG_DEBUG("Restore Children returns %d", result_code);
          return result_code;
        }
      } else if (UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code) {
        UPLL_LOG_DEBUG("UpdateConfigDB Failed %d", result_code);
        return result_code;
      }
    }
  } else {
    result_code = UPLL_RC_ERR_NO_SUCH_INSTANCE;
  }
  //  create a record in CANDIDATE DB
  VrtIfMoMgr *vrtifmgr =
      reinterpret_cast<VrtIfMoMgr *>(const_cast<MoManager *>(GetMoManager(
                  UNC_KT_VRT_IF)));
  ConfigKeyVal *ckv = NULL;
  InterfacePortMapInfo flags = kVlinkPortMapNotConfigured;
  result_code = vrtifmgr->GetChildConfigKey(ckv, NULL);
  key_vrt_if_flowfilter_entry_t *ff_key = reinterpret_cast
      <key_vrt_if_flowfilter_entry_t *>(ikey->get_key());
  key_vrt_if_t *vrtif_key = reinterpret_cast<key_vrt_if_t *>(ckv->get_key());

  uuu::upll_strncpy(vrtif_key->vrt_key.vtn_key.vtn_name,
                    ff_key->flowfilter_key.if_key.vrt_key.vtn_key.vtn_name,
                    kMaxLenVtnName + 1);

  uuu::upll_strncpy(vrtif_key->vrt_key.vrouter_name,
                    ff_key->flowfilter_key.if_key.vrt_key.vrouter_name,
                    kMaxLenVtnName + 1);

  uuu::upll_strncpy(vrtif_key->if_name,
                    ff_key->flowfilter_key.if_key.if_name,
                    kMaxLenInterfaceName + 1);

  uint8_t* vexternal = reinterpret_cast<uint8_t*>
      (ConfigKeyVal::Malloc(kMaxLenVnodeName + 1));
  uint8_t* vex_if = reinterpret_cast<uint8_t*>
      (ConfigKeyVal::Malloc(kMaxLenInterfaceName + 1));
  result_code = vrtifmgr->GetVexternal(ckv, req->datatype, dmi,
                                       vexternal, vex_if, flags);
  if (UPLL_RC_SUCCESS != result_code) {
    DELETE_IF_NOT_NULL(ckv);
    free(vexternal);
    free(vex_if);
    return result_code;
  }
  DELETE_IF_NOT_NULL(ckv);
  uint8_t flag_port_map = 0;
  GET_USER_DATA_FLAGS(ikey, flag_port_map);
  if (flags & kVlinkConfigured) {
    flag_port_map = flag_port_map | SET_FLAG_VLINK;
  }
  free(vexternal);
  free(vex_if);
  SET_USER_DATA_FLAGS(ikey, flag_port_map);
  if (flags & kVlinkConfigured) {
    if (flowfilter_val->valid[UPLL_IDX_FLOWLIST_NAME_FFE] == UNC_VF_VALID) {
      FlowListMoMgr *mgr = reinterpret_cast<FlowListMoMgr *>
          (const_cast<MoManager *> (GetMoManager(UNC_KT_FLOWLIST)));
      result_code = mgr->AddFlowListToController(
          reinterpret_cast<char *>(flowfilter_val->flowlist_name), dmi,
          reinterpret_cast<char *> (ctrlr_id), req->datatype, UNC_OP_CREATE);
      if (result_code != UPLL_RC_SUCCESS) {
        UPLL_LOG_DEBUG("Reference Count Updation Fails %d", result_code);
        return result_code;
      }
    }
  }
  DbSubOp dbop = { kOpNotRead, kOpMatchNone, kOpInOutDomain
    | kOpInOutCtrlr | kOpInOutFlag };
  result_code = UpdateConfigDB(ikey, req->datatype, UNC_OP_CREATE, dmi,
                               &dbop, MAINTBL);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_DEBUG("Unable to update CandidateDB %d", result_code);
  }

  return result_code;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::GetControllerDomainID(
    ConfigKeyVal *ikey,
    upll_keytype_datatype_t dt_type,
    DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  controller_domain ctrlr_dom;
  memset(&ctrlr_dom, 0, sizeof(controller_domain));
  VrtIfMoMgr *mgrvrtif =
      reinterpret_cast<VrtIfMoMgr *>(const_cast<MoManager *>(GetMoManager(
                  UNC_KT_VRT_IF)));
  if (NULL == mgrvrtif) {
    UPLL_LOG_DEBUG("mgrvrtif is NULL");
    return UPLL_RC_ERR_GENERIC;
  }
  ConfigKeyVal *ckv = NULL;
  result_code = mgrvrtif->GetChildConfigKey(ckv, NULL);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_DEBUG("Unable to get the ParentConfigKey, resultcode=%d",
                   result_code);
    return result_code;
  }

  key_vrt_if_flowfilter_entry_t *temp_key =
      reinterpret_cast<key_vrt_if_flowfilter_entry_t*>(ikey->get_key());

  key_vrt_if_t *vrt_if_key = reinterpret_cast<key_vrt_if_t*>(ckv->get_key());

  uuu::upll_strncpy(vrt_if_key->vrt_key.vtn_key.vtn_name,
                    temp_key->flowfilter_key.if_key.vrt_key.vtn_key.vtn_name,
                    kMaxLenVtnName + 1);
  uuu::upll_strncpy(vrt_if_key->vrt_key.vrouter_name,
                    temp_key->flowfilter_key.if_key.vrt_key.vrouter_name,
                    kMaxLenVnodeName + 1);

  uuu::upll_strncpy(vrt_if_key->if_name,
                    temp_key->flowfilter_key.if_key.if_name,
                    kMaxLenInterfaceName + 1);


  ConfigKeyVal *vrt_key = NULL;
  result_code = mgrvrtif->GetParentConfigKey(vrt_key, ckv);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_DEBUG("GetParentConfigKey Failed, err %d", result_code);
    DELETE_IF_NOT_NULL(ckv);
    return result_code;
  }

  result_code = mgrvrtif->GetControllerDomainId(vrt_key, dt_type,
                                                &ctrlr_dom, dmi);
  if ((result_code != UPLL_RC_SUCCESS) || (ctrlr_dom.ctrlr == NULL)
      || (ctrlr_dom.domain == NULL)) {
    DELETE_IF_NOT_NULL(vrt_key);
    DELETE_IF_NOT_NULL(ckv);
    UPLL_LOG_INFO("GetControllerDomainId error err code(%d)", result_code);
    return result_code;
  }

  UPLL_LOG_DEBUG("ctrlrid %s, domainid %s",
                 ctrlr_dom.ctrlr, ctrlr_dom.domain);
  uint8_t temp_flag = 0;
  GET_USER_DATA_FLAGS(ikey, temp_flag);
  SET_USER_DATA(ikey, vrt_key);
  SET_USER_DATA_FLAGS(ikey, temp_flag);
  //  SET_USER_DATA_CTRLR_DOMAIN(ikey, ctrlr_dom);

  DELETE_IF_NOT_NULL(vrt_key);
  DELETE_IF_NOT_NULL(ckv);
  return result_code;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::GetParentConfigKey(ConfigKeyVal *&okey,
                                                        ConfigKeyVal *ikey) {
  UPLL_FUNC_TRACE;

  if (!ikey) {
    UPLL_LOG_DEBUG(" Input Key is NULL");
    return UPLL_RC_ERR_GENERIC;
  }
  DELETE_IF_NOT_NULL(okey);
  unc_key_type_t ikey_type = ikey->get_key_type();
  if (ikey_type != UNC_KT_VRTIF_FLOWFILTER_ENTRY) {
    UPLL_LOG_DEBUG(" Invalid key type received. Key type - %d", ikey_type);
    return UPLL_RC_ERR_GENERIC;
  }

  key_vrt_if_flowfilter_entry_t *pkey =
      reinterpret_cast<key_vrt_if_flowfilter_entry_t*>(ikey->get_key());
  if (!pkey) {
    UPLL_LOG_DEBUG(" Input vrt if flow filter entry key is NULL ");
    return UPLL_RC_ERR_GENERIC;
  }

  key_vrt_if_flowfilter_t *vrt_if_ff_key =
      reinterpret_cast<key_vrt_if_flowfilter_t*>
      (ConfigKeyVal::Malloc(sizeof(key_vrt_if_flowfilter_t)));

  uuu::upll_strncpy(vrt_if_ff_key->if_key.vrt_key.vtn_key.vtn_name,
                    reinterpret_cast<key_vrt_if_flowfilter_entry_t *>
                    (pkey)->flowfilter_key.if_key.vrt_key.vtn_key.vtn_name,
                    kMaxLenVtnName + 1);
  uuu::upll_strncpy(vrt_if_ff_key->if_key.vrt_key.vrouter_name,
                    reinterpret_cast<key_vrt_if_flowfilter_entry_t *>
                    (pkey)->flowfilter_key.if_key.vrt_key.vrouter_name,
                    kMaxLenVnodeName + 1);
  uuu::upll_strncpy(vrt_if_ff_key->if_key.if_name,
                    reinterpret_cast<key_vrt_if_flowfilter_entry_t *>
                    (pkey)->flowfilter_key.if_key.if_name,
                    kMaxLenInterfaceName + 1);
  vrt_if_ff_key->direction = reinterpret_cast<key_vrt_if_flowfilter_entry_t *>
      (pkey)->flowfilter_key.direction;
  okey = new ConfigKeyVal(UNC_KT_VRTIF_FLOWFILTER,
                          IpctSt::kIpcStKeyVrtIfFlowfilter,
                          vrt_if_ff_key, NULL);
  SET_USER_DATA(okey, ikey);
  return UPLL_RC_SUCCESS;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::ConstructReadDetailResponse(
    ConfigKeyVal *ikey,
    ConfigKeyVal *drv_resp_ckv,
    ConfigKeyVal **okey) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  ConfigKeyVal *tmp_okey = NULL;

  result_code =  GetChildConfigKey(tmp_okey, ikey);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_DEBUG("GetChildConfigKey failed err code (%d)", result_code);
    return result_code;
  }
  tmp_okey->AppendCfgVal(drv_resp_ckv->GetCfgValAndUnlink());
  if (*okey == NULL) {
    *okey = tmp_okey;
  } else {
    (*okey)->AppendCfgKeyVal(tmp_okey);
  }
  return UPLL_RC_SUCCESS;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::DeleteChildrenPOM(
    ConfigKeyVal *ikey, upll_keytype_datatype_t dt_type,
    DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  uint8_t *ctrlr_id = NULL;
  upll_rc_t result_code = UPLL_RC_ERR_GENERIC;
  //  uint8_t rename = 0;
  if (NULL == ikey && NULL == dmi) return result_code;

  ConfigKeyVal *temp_okey = NULL;
  ConfigKeyVal *okey = NULL;
  result_code = GetChildConfigKey(temp_okey, ikey);
  if (UPLL_RC_SUCCESS != result_code) {
    return result_code;
  }
  DbSubOp dbop = { kOpReadMultiple, kOpMatchNone,
    kOpInOutCtrlr|kOpInOutDomain|kOpInOutFlag };
  result_code = ReadConfigDB(temp_okey, UPLL_DT_CANDIDATE,
                             UNC_OP_READ, dbop, dmi, MAINTBL);
  if (result_code != UPLL_RC_SUCCESS) {
    DELETE_IF_NOT_NULL(temp_okey);
    if (UPLL_RC_ERR_NO_SUCH_INSTANCE == result_code) {
      UPLL_LOG_DEBUG("UPLL_RC_ERR_NO_SUCH_INSTANCE");
      return UPLL_RC_SUCCESS;
    }
    UPLL_LOG_DEBUG("Unable to read configuration from CandidateDb");
    return result_code;
  }
  okey = temp_okey;
  while (NULL != okey) {
    GET_USER_DATA_CTRLR(okey, ctrlr_id);
    val_flowfilter_entry_t *flowfilter_val =
        reinterpret_cast<val_flowfilter_entry_t *> (GetVal(okey));
    if (flowfilter_val->valid[UPLL_IDX_FLOWLIST_NAME_FFE] == UNC_VF_VALID) {
      uint8_t flag_port_map = 0;
      GET_USER_DATA_FLAGS(okey, flag_port_map);
      if (flag_port_map & SET_FLAG_VLINK) {
        FlowListMoMgr *mgr = reinterpret_cast<FlowListMoMgr *>
            (const_cast<MoManager *> (GetMoManager(UNC_KT_FLOWLIST)));
        result_code = mgr->AddFlowListToController(
            reinterpret_cast<char*>(flowfilter_val->flowlist_name), dmi,
            reinterpret_cast<char *>(ctrlr_id), dt_type, UNC_OP_DELETE);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG(" Send delete request to flowlist failed."
                         "err code(%d)",
                         result_code);
          DELETE_IF_NOT_NULL(temp_okey);
          return result_code;
        }
      }
    }
    result_code = UpdateConfigDB(okey, dt_type, UNC_OP_DELETE, dmi,
                                 MAINTBL);
    if (result_code != UPLL_RC_SUCCESS) {
      UPLL_LOG_DEBUG("Failed to delete the vrt if flowfilter entry , err %d",
                     result_code);
      DELETE_IF_NOT_NULL(temp_okey);
      return result_code;
    }
    okey = okey->get_next_cfg_key_val();
  }
  DELETE_IF_NOT_NULL(temp_okey);
  return UPLL_RC_SUCCESS;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::AuditUpdateController(
    unc_key_type_t keytype,
    const char *ctrlr_id,
    uint32_t session_id,
    uint32_t config_id,
    uuc::UpdateCtrlrPhase phase,
    DalDmlIntf *dmi,
    ConfigKeyVal **err_ckv,
    KTxCtrlrAffectedState *ctrlr_affected) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  DalResultCode db_result = uud::kDalRcSuccess;
  MoMgrTables tbl  = MAINTBL;
  controller_domain_t ctrlr_dom;
  ctrlr_dom.ctrlr = NULL;
  ctrlr_dom.domain = NULL;
  ConfigKeyVal  *ckv_running_db = NULL;
  ConfigKeyVal  *ckv_audit_db = NULL;
  ConfigKeyVal  *ckv_driver_req = NULL;
  ConfigKeyVal  *ckv_audit_dup_db = NULL;
  DalCursor *cursor = NULL;
  uint8_t db_flag = 0;
  uint8_t *ctrlr = reinterpret_cast<uint8_t *>(const_cast<char *>(ctrlr_id));
  /* decides whether to retrieve from controller table or main table */
  DbSubOp dbop = { kOpReadSingle, kOpMatchNone, kOpInOutCtrlr | kOpInOutDomain};
  // GET_TABLE_TYPE(keytype, tbl);
  unc_keytype_operation_t op = (phase == uuc::kUpllUcpCreate)?UNC_OP_CREATE:
      ((phase == uuc::kUpllUcpUpdate)?UNC_OP_UPDATE:
       ((phase == uuc::kUpllUcpDelete)?UNC_OP_DELETE:UNC_OP_INVALID));

  unc_keytype_operation_t op1 = op;
  if (phase == uuc::kUpllUcpDelete2)
    return result_code;
  /* retreives the delta of running and audit configuration */
  UPLL_LOG_DEBUG("Operation is %d", op);
  result_code = DiffConfigDB(UPLL_DT_RUNNING, UPLL_DT_AUDIT, op,
                             ckv_running_db, ckv_audit_db,
                             &cursor, dmi, ctrlr, tbl);
  if (UPLL_RC_ERR_NO_SUCH_INSTANCE == result_code) {
    UPLL_LOG_DEBUG("No more diff found for operation %d", op);
    DELETE_IF_NOT_NULL(ckv_running_db);
    DELETE_IF_NOT_NULL(ckv_audit_db);
    return UPLL_RC_SUCCESS;
  }
  if (UPLL_RC_SUCCESS != result_code) {
    UPLL_LOG_DEBUG("DiffConfigDB failed - %d", result_code);
    DELETE_IF_NOT_NULL(ckv_running_db);
    DELETE_IF_NOT_NULL(ckv_audit_db);
    return result_code;
  }
  while (uud::kDalRcSuccess == (db_result = dmi->GetNextRecord(cursor))) {
    op1 = op;
    if (phase != uuc::kUpllUcpDelete) {
      uint8_t *db_ctrlr = NULL;
      GET_USER_DATA_CTRLR(ckv_running_db, db_ctrlr);
      UPLL_LOG_DEBUG("db ctrl_id and audit ctlr_id are  %s %s",
                     db_ctrlr, ctrlr_id);
      //  Skipping the controller ID if the controller id in DB and
      //  controller id available for Audit are not the same
      if (db_ctrlr && strncmp(reinterpret_cast<const char *>(db_ctrlr),
                              reinterpret_cast<const char *>(ctrlr_id),
                              strlen(reinterpret_cast<const char *>
                                     (ctrlr_id)) + 1)) {
        continue;
      }
    }
    /* ignore records of another controller for create and update operation */
    switch (phase) {
      case uuc::kUpllUcpDelete:
        UPLL_LOG_TRACE("Deleted record is %s ",
                       ckv_running_db->ToStrAll().c_str());
        result_code = GetChildConfigKey(ckv_driver_req, ckv_running_db);
        UPLL_LOG_TRACE("ckv_driver_req in delete is %s",
                       ckv_driver_req->ToStrAll().c_str());
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("GetChildConfigKey failed. err_code & phase %d %d",
                         result_code, phase);
          DELETE_IF_NOT_NULL(ckv_running_db);
          DELETE_IF_NOT_NULL(ckv_audit_db);
          if (cursor)
            dmi->CloseCursor(cursor, true);
          return result_code;
        }
        if (ckv_driver_req->get_cfg_val()) {
          UPLL_LOG_DEBUG("Invalid param");
          DELETE_IF_NOT_NULL(ckv_driver_req);
          DELETE_IF_NOT_NULL(ckv_running_db);
          DELETE_IF_NOT_NULL(ckv_audit_db);
          if (cursor)
            dmi->CloseCursor(cursor, true);
          return UPLL_RC_ERR_GENERIC;
        }
        result_code = ReadConfigDB(ckv_driver_req, UPLL_DT_AUDIT, UNC_OP_READ,
                                   dbop, dmi, MAINTBL);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("Returning error %d", result_code);
          DELETE_IF_NOT_NULL(ckv_driver_req);
          DELETE_IF_NOT_NULL(ckv_running_db);
          DELETE_IF_NOT_NULL(ckv_audit_db);
          if (cursor)
            dmi->CloseCursor(cursor, true);
          return UPLL_RC_ERR_GENERIC;
        }
        break;
      case uuc::kUpllUcpCreate:
        UPLL_LOG_TRACE("Created  record is %s ",
                       ckv_running_db->ToStrAll().c_str());
        result_code = DupConfigKeyVal(ckv_driver_req, ckv_running_db, MAINTBL);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("DupConfigKeyVal failed. err_code & phase %d %d",
                         result_code, phase);
          DELETE_IF_NOT_NULL(ckv_running_db);
          DELETE_IF_NOT_NULL(ckv_audit_db);
          if (cursor)
            dmi->CloseCursor(cursor, true);
          return result_code;
        }
        break;
      case uuc::kUpllUcpUpdate:
        ckv_audit_dup_db = NULL;
        ckv_driver_req = NULL;
        UPLL_LOG_TRACE("UpdateRecord  record  is %s ",
                       ckv_running_db->ToStrAll().c_str());
        UPLL_LOG_TRACE("UpdateRecord  record  is %s ",
                       ckv_audit_db->ToStrAll().c_str());
        result_code = DupConfigKeyVal(ckv_driver_req, ckv_running_db, MAINTBL);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("DupConfigKeyVal failed for running record."
                         "err_code & phase %d %d", result_code, phase);
          DELETE_IF_NOT_NULL(ckv_running_db);
          DELETE_IF_NOT_NULL(ckv_audit_db);
          if (cursor)
            dmi->CloseCursor(cursor, true);
          return result_code;
        }
        result_code = DupConfigKeyVal(ckv_audit_dup_db, ckv_audit_db, MAINTBL);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("DupConfigKeyVal failed for audit record. "
                         "err_code & phase %d %d", result_code, phase);
          DELETE_IF_NOT_NULL(ckv_running_db);
          DELETE_IF_NOT_NULL(ckv_audit_db);
          DELETE_IF_NOT_NULL(ckv_driver_req);
          if (cursor)
            dmi->CloseCursor(cursor, true);
          return result_code;
        }
        break;
      default:
        UPLL_LOG_DEBUG("Invalid operation %d", phase);
        return UPLL_RC_ERR_NO_SUCH_OPERATION;
        break;
    }

    GET_USER_DATA_CTRLR_DOMAIN(ckv_driver_req, ctrlr_dom);
    if (ctrlr_dom.ctrlr == NULL) {
      DELETE_IF_NOT_NULL(ckv_driver_req);
      DELETE_IF_NOT_NULL(ckv_audit_dup_db);
      DELETE_IF_NOT_NULL(ckv_running_db);
      DELETE_IF_NOT_NULL(ckv_audit_db);
      if (cursor)
        dmi->CloseCursor(cursor, true);
      return UPLL_RC_ERR_GENERIC;
    }

    GET_USER_DATA_FLAGS(ckv_driver_req, db_flag);
    //  If vlink flag is not set at running and the operation is update
    //  then vlink is deleted in the update phase from UNC
    //  hence flowfilter seq no also should get deleted from controller
    //  hence sending the delete request to the controller driver
    if (SET_FLAG_VLINK & db_flag) {
      //  Continue with further operations
    } else {
      if (UNC_OP_UPDATE == op1) {
        op1 = UNC_OP_DELETE;
      } else {
        //  No Vlink Configured, Configuration is not
        //  sent to driver
        DELETE_IF_NOT_NULL(ckv_driver_req);
        DELETE_IF_NOT_NULL(ckv_audit_dup_db);
        continue;
      }
    }

    if (UNC_OP_UPDATE == op1) {
      void *running_val = NULL;
      bool invalid_attr = false;
      running_val = GetVal(ckv_driver_req);
      invalid_attr = FilterAttributes(running_val,
                                      GetVal(ckv_audit_dup_db),
                                      false,
                                      UNC_OP_UPDATE);
      if (invalid_attr) {
        DELETE_IF_NOT_NULL(ckv_driver_req);
        DELETE_IF_NOT_NULL(ckv_audit_dup_db);
        //  Assuming that the diff found only in ConfigStatus
        //  Setting the   value as OnlyCSDiff in the out
        //  parameter ctrlr_affected
        //  The value Configdiff should be given more
        //  priority than the value
        //  onlycs .
        //  So  If the out parameter ctrlr_affected
        //  has already value as configdiff
        //   then dont change the value
        if (*ctrlr_affected != uuc::kCtrlrAffectedConfigDiff) {
          UPLL_LOG_INFO("Setting the ctrlr_affected to OnlyCSDiff");
          *ctrlr_affected = uuc::kCtrlrAffectedOnlyCSDiff;
        }
        continue;
      }
    }

    DELETE_IF_NOT_NULL(ckv_audit_dup_db);
    upll_keytype_datatype_t dt_type = (op1 == UNC_OP_DELETE)?
        UPLL_DT_AUDIT : UPLL_DT_RUNNING;

    result_code = GetRenamedControllerKey(ckv_driver_req, UPLL_DT_RUNNING,
                                          dmi, &ctrlr_dom);
    if (result_code != UPLL_RC_SUCCESS && result_code !=
        UPLL_RC_ERR_NO_SUCH_INSTANCE) {
      UPLL_LOG_DEBUG(" GetRenamedControllerKey failed err code(%d)",
                     result_code);
      DELETE_IF_NOT_NULL(ckv_driver_req);
      DELETE_IF_NOT_NULL(ckv_running_db);
      DELETE_IF_NOT_NULL(ckv_audit_db);
      if (cursor)
        dmi->CloseCursor(cursor, true);
      return result_code;
    }


    pfcdrv_val_flowfilter_entry_t *pfc_val = reinterpret_cast
        <pfcdrv_val_flowfilter_entry_t *>
        (ConfigKeyVal::Malloc(sizeof(pfcdrv_val_flowfilter_entry_t)));

    pfc_val->valid[PFCDRV_IDX_VAL_VBRIF_VEXTIF_FFE] = UNC_VF_VALID;
    pfc_val->valid[PFCDRV_IDX_FLOWFILTER_ENTRY_FFE] = UNC_VF_VALID;
    pfc_val->val_vbrif_vextif.valid[PFCDRV_IDX_INTERFACE_TYPE] = UNC_VF_VALID;
    pfc_val->val_vbrif_vextif.interface_type = PFCDRV_IF_TYPE_VBRIF;

    val_flowfilter_entry_t* val = reinterpret_cast<val_flowfilter_entry_t *>
        (GetVal(ckv_driver_req));
    memcpy(&pfc_val->val_ff_entry, val, sizeof(val_flowfilter_entry_t));

    ckv_driver_req->SetCfgVal(new ConfigVal(
            IpctSt::kIpcStPfcdrvValFlowfilterEntry,
            pfc_val));

    IpcResponse ipc_response;
    memset(&ipc_response, 0, sizeof(IpcResponse));
    IpcRequest ipc_req;
    memset(&ipc_req, 0, sizeof(IpcRequest));
    ipc_req.header.clnt_sess_id = session_id;
    ipc_req.header.config_id = config_id;
    ipc_req.header.operation = op1;
    ipc_req.header.datatype = UPLL_DT_CANDIDATE;
    ipc_req.ckv_data = ckv_driver_req;
    if (!IpcUtil::SendReqToDriver((const char *)ctrlr_dom.ctrlr,
                                  reinterpret_cast<char *>
                                  (ctrlr_dom.domain), PFCDRIVER_SERVICE_NAME,
                                  PFCDRIVER_SVID_LOGICAL,
                                  &ipc_req, true,
                                  &ipc_response)) {
      UPLL_LOG_INFO("Request to driver for Key %d for controller %s failed ",
                    ckv_driver_req->get_key_type(),
                    reinterpret_cast<char *>(ctrlr_dom.ctrlr));

      DELETE_IF_NOT_NULL(ckv_driver_req);
      if (cursor)
        dmi->CloseCursor(cursor, true);
      DELETE_IF_NOT_NULL(ckv_running_db);
      DELETE_IF_NOT_NULL(ckv_audit_db);
      return UPLL_RC_ERR_GENERIC;
    }
    if (ipc_response.header.result_code != UPLL_RC_SUCCESS) {
      UPLL_LOG_DEBUG("driver return failure err_code is %d",
                     ipc_response.header.result_code);
      *err_ckv = ckv_running_db;
      if (phase != uuc::kUpllUcpDelete) {
        ConfigKeyVal *resp = NULL;

        result_code = GetChildConfigKey(resp, ipc_response.ckv_data);
        if (!resp || result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("DupConfigKeyVal failed for ipc"
                         "response ckv err_code %d",
                         result_code);
          DELETE_IF_NOT_NULL(ipc_response.ckv_data);
          DELETE_IF_NOT_NULL(resp);
          DELETE_IF_NOT_NULL(ckv_driver_req);
          DELETE_IF_NOT_NULL(ckv_running_db);
          DELETE_IF_NOT_NULL(ckv_audit_db);
          if (cursor)
            dmi->CloseCursor(cursor, true);
          return result_code;
        }

        pfcdrv_val_flowfilter_entry_t *pfc_val_ff = reinterpret_cast
            <pfcdrv_val_flowfilter_entry_t *>
            (GetVal(ipc_response.ckv_data));
        if (NULL == pfc_val_ff) {
          UPLL_LOG_DEBUG("pfcdrv_val_flowfilter_entry_t is NULL");
          DELETE_IF_NOT_NULL(resp);
          DELETE_IF_NOT_NULL(ckv_driver_req);
          DELETE_IF_NOT_NULL(ipc_response.ckv_data);
          DELETE_IF_NOT_NULL(ckv_running_db);
          DELETE_IF_NOT_NULL(ckv_audit_db);
          if (cursor)
            dmi->CloseCursor(cursor, true);
          return UPLL_RC_ERR_GENERIC;
        }
        val_flowfilter_entry_t* val_ff = reinterpret_cast
            <val_flowfilter_entry_t *>
            (ConfigKeyVal::Malloc(sizeof(val_flowfilter_entry_t)));
        memcpy(val_ff, &pfc_val_ff->val_ff_entry,
               sizeof(val_flowfilter_entry_t));
        resp->AppendCfgVal(IpctSt::kIpcStValFlowfilterEntry, val_ff);
        result_code = UpdateAuditConfigStatus(UNC_CS_INVALID,
                                              phase,
                                              resp,
                                              dmi);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_TRACE("Update Audit config status failed %d",
                         result_code);
          DELETE_IF_NOT_NULL(resp);
          DELETE_IF_NOT_NULL(ckv_driver_req);
          DELETE_IF_NOT_NULL(ipc_response.ckv_data);
          DELETE_IF_NOT_NULL(ckv_running_db);
          DELETE_IF_NOT_NULL(ckv_audit_db);
          if (cursor)
            dmi->CloseCursor(cursor, true);
          return result_code;
        }
        DbSubOp dbop = { kOpNotRead, kOpMatchNone, kOpInOutCs };
        result_code = UpdateConfigDB(resp, dt_type, UNC_OP_UPDATE,
                                     dmi, &dbop, tbl);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("UpdateConfigDB failed for ipc response"
                         "ckv err_code %d",
                         result_code);
          DELETE_IF_NOT_NULL(resp);
          DELETE_IF_NOT_NULL(ckv_driver_req);
          DELETE_IF_NOT_NULL(ipc_response.ckv_data);
          DELETE_IF_NOT_NULL(ckv_running_db);
          DELETE_IF_NOT_NULL(ckv_audit_db);
          if (cursor)
            dmi->CloseCursor(cursor, true);
          return result_code;
        }
        DELETE_IF_NOT_NULL(resp);
      }
      return ipc_response.header.result_code;
    }
    DELETE_IF_NOT_NULL(ckv_driver_req);
    DELETE_IF_NOT_NULL(ipc_response.ckv_data);
    //  *ctrlr_affected = true;
    if (*ctrlr_affected == uuc::kCtrlrAffectedOnlyCSDiff) {
      UPLL_LOG_INFO("Reset ctrlr state from OnlyCSDiff to ConfigDiff");
    }
    UPLL_LOG_DEBUG("Setting the ctrlr_affected to ConfigDiff");
    *ctrlr_affected = uuc::kCtrlrAffectedConfigDiff;
  }
  if (cursor)
    dmi->CloseCursor(cursor, true);
  if (uud::kDalRcSuccess != db_result) {
    UPLL_LOG_DEBUG("GetNextRecord from database failed  - %d", db_result);
    result_code =  DalToUpllResCode(db_result);
  }
  DELETE_IF_NOT_NULL(ckv_running_db);
  DELETE_IF_NOT_NULL(ckv_audit_db);
  result_code = (result_code == UPLL_RC_ERR_NO_SUCH_INSTANCE)
      ? UPLL_RC_SUCCESS : result_code;
  return result_code;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::SetValidAudit(ConfigKeyVal *&ikey) {
  UPLL_FUNC_TRACE;
  val_flowfilter_entry_t *val = reinterpret_cast
      <val_flowfilter_entry_t *>(GetVal(ikey));
  if (NULL == val) {
    UPLL_LOG_DEBUG("val is NULL");
    return UPLL_RC_ERR_GENERIC;
  }
  for (unsigned int loop = 0;
       loop < sizeof(val->valid) / sizeof(val->valid[0]);
       ++loop) {
    if (val->valid[loop] == UNC_VF_VALID) {
      val->cs_attr[loop] = UNC_CS_APPLIED;
    } else if (val->valid[loop] == UNC_VF_INVALID) {
      val->cs_attr[loop] = UNC_CS_NOT_APPLIED;
    }
  }
  val->cs_row_status = UNC_CS_APPLIED;
  return UPLL_RC_SUCCESS;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::CreateAuditMoImpl(ConfigKeyVal *ikey,
                                                       DalDmlIntf *dmi,
                                                       const char *ctrlr_id) {
  UPLL_FUNC_TRACE;
  uint8_t flags = 0;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  UPLL_LOG_TRACE(" ikey is %s", ikey->ToStrAll().c_str());
  uint8_t *controller_id = reinterpret_cast<uint8_t *>(
      const_cast<char *>(ctrlr_id));

  /* check if object is renamed in the corresponding Rename Tbl
   *    * if "renamed"  create the object by the UNC name.
   *       * else - create using the controller name.
   *          */
  result_code = GetRenamedUncKey(ikey, UPLL_DT_RUNNING, dmi, controller_id);
  if (result_code != UPLL_RC_SUCCESS && result_code !=
      UPLL_RC_ERR_NO_SUCH_INSTANCE) {
    UPLL_LOG_DEBUG("GetRenamedUncKey Failed err_code %d", result_code);
    return result_code;
  }

  pfcdrv_val_flowfilter_entry_t *pfc_val =
      reinterpret_cast<pfcdrv_val_flowfilter_entry_t *> (GetVal(ikey));
  if (pfc_val == NULL) {
    UPLL_LOG_DEBUG("Driver Structure Empty");
    return UPLL_RC_ERR_GENERIC;
  }

  if (pfc_val->val_vbrif_vextif.interface_type == PFCDRV_IF_TYPE_VBRIF) {
    flags = SET_FLAG_VLINK;
  } else {
    flags = 0;
  }

  ConfigKeyVal *okey = NULL;
  result_code = GetChildConfigKey(okey, ikey);
  if (!okey || result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_DEBUG("GetChildConfigkey Failed:%d", result_code);
    DELETE_IF_NOT_NULL(okey);
    return result_code;
  }

  val_flowfilter_entry_t * val_ff_entry = NULL;
  val_ff_entry = reinterpret_cast<val_flowfilter_entry_t *>
      (ConfigKeyVal::Malloc(sizeof(val_flowfilter_entry_t)));

  memcpy(val_ff_entry, &pfc_val->val_ff_entry, sizeof(val_flowfilter_entry_t));
  okey->AppendCfgVal(IpctSt::kIpcStValFlowfilterEntry, val_ff_entry);
  SET_USER_DATA_FLAGS(okey, flags);

  controller_domain ctrlr_dom;
  memset(&ctrlr_dom, 0, sizeof(ctrlr_dom));
  result_code = GetControllerDomainID(okey, UPLL_DT_AUDIT, dmi);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_DEBUG("Failed to Get the Controller Domain details, err:%d",
                   result_code);
    DELETE_IF_NOT_NULL(okey);
    return result_code;
  }

  GET_USER_DATA_CTRLR_DOMAIN(okey, ctrlr_dom);
  UPLL_LOG_DEBUG("ctrlrid %s, domainid %s",
                 ctrlr_dom.ctrlr, ctrlr_dom.domain);

  FlowListMoMgr *mgr = reinterpret_cast<FlowListMoMgr *>
      (const_cast<MoManager *> (GetMoManager(UNC_KT_FLOWLIST)));
  if (mgr == NULL) {
    UPLL_LOG_DEBUG("Invalid FlowListMoMgr Instance");
    DELETE_IF_NOT_NULL(okey);
    return UPLL_RC_ERR_GENERIC;
  }

  if (pfc_val->val_ff_entry.valid[UPLL_IDX_FLOWLIST_NAME_FFE] == UNC_VF_VALID) {
    result_code = mgr->AddFlowListToController(
        reinterpret_cast<char *>
        (pfc_val->val_ff_entry.flowlist_name), dmi,
        reinterpret_cast<char *> (const_cast<char *>(ctrlr_id)),
        UPLL_DT_AUDIT,
        UNC_OP_CREATE);

    if (result_code != UPLL_RC_SUCCESS) {
      UPLL_LOG_DEBUG("Reference Count Updation Fails %d", result_code);
      DELETE_IF_NOT_NULL(okey);
      return result_code;
    }
  }

  UPLL_LOG_TRACE("ikey After GetRenamedUncKey %s", ikey->ToStrAll().c_str());
  result_code = SetValidAudit(ikey);
  if (UPLL_RC_SUCCESS != result_code) {
    DELETE_IF_NOT_NULL(okey);
    return result_code;
  }
  result_code = UpdateConfigDB(okey,
                               UPLL_DT_AUDIT,
                               UNC_OP_CREATE,
                               dmi,
                               MAINTBL);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_DEBUG("UpdateConfigDB Failed err_code %d", result_code);
    DELETE_IF_NOT_NULL(okey);
    return result_code;
  }
  DELETE_IF_NOT_NULL(okey);
  return UPLL_RC_SUCCESS;
}


bool VrtIfFlowFilterEntryMoMgr::FilterAttributes(void *&val1,
                                                 void *val2,
                                                 bool copy_to_running,
                                                 unc_keytype_operation_t op) {
  UPLL_FUNC_TRACE;
  if (op != UNC_OP_CREATE)
    return CompareValidValue(val1, val2, copy_to_running);
  return false;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::UpdateConfigStatus(
    ConfigKeyVal *ikey,
    unc_keytype_operation_t op,
    uint32_t driver_result,
    ConfigKeyVal *upd_key,
    DalDmlIntf *dmi,
    ConfigKeyVal *ctrlr_key) {
  UPLL_FUNC_TRACE;
  ConfigKeyVal * vrt_ffe_run_key = NULL;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  val_flowfilter_entry_t *ffe_val = NULL, *val_main = NULL;
  unc_keytype_configstatus_t cs_status =
      (driver_result == UPLL_RC_SUCCESS) ? UNC_CS_APPLIED : UNC_CS_NOT_APPLIED;
  ffe_val = reinterpret_cast<val_flowfilter_entry_t *>(GetVal(ikey));
  if (ffe_val == NULL) return UPLL_RC_ERR_GENERIC;
  if (op == UNC_OP_CREATE) {
    ffe_val->cs_row_status = cs_status;
  } else if (op == UNC_OP_UPDATE) {
    result_code = GetChildConfigKey(vrt_ffe_run_key, ikey);
    if (result_code != UPLL_RC_SUCCESS) {
      UPLL_LOG_DEBUG("GetChildConfigKey is failed resultcode=%d",
                     result_code);
      return result_code;
    }
    DbSubOp dbop_maintbl = { kOpReadSingle, kOpMatchNone,
      kOpInOutFlag |kOpInOutCs };
    result_code = ReadConfigDB(vrt_ffe_run_key, UPLL_DT_RUNNING  ,
                               UNC_OP_READ, dbop_maintbl, dmi, MAINTBL);
    if (result_code != UPLL_RC_SUCCESS) {
      UPLL_LOG_DEBUG("Unable to read configuration from CandidateDb");
      DELETE_IF_NOT_NULL(vrt_ffe_run_key);
      return result_code;
    }
    val_main = reinterpret_cast
        <val_flowfilter_entry_t *>(GetVal(vrt_ffe_run_key));
    for (unsigned int loop = 0; loop < sizeof(val_main->valid)/
         sizeof(val_main->valid[0]); ++loop) {
      ffe_val->cs_attr[loop] = val_main->cs_attr[loop];
    }
    void *ffeval = reinterpret_cast<void *>(ffe_val);
    CompareValidValue(ffeval, GetVal(vrt_ffe_run_key), true);
  } else {
    return UPLL_RC_ERR_GENERIC;
  }
  UPLL_LOG_TRACE("%s", (ikey->ToStrAll()).c_str());
  val_flowfilter_entry_t *ffe_val2 =
      reinterpret_cast<val_flowfilter_entry_t *>(GetVal(upd_key));
  if (UNC_OP_UPDATE == op) {
    UPLL_LOG_TRACE("%s", (upd_key->ToStrAll()).c_str());
    ffe_val->cs_row_status = ffe_val2->cs_row_status;
  }
  for (unsigned int loop = 0;
       loop < sizeof(ffe_val->valid) / sizeof(ffe_val->valid[0]); ++loop) {
    /* Setting CS to the not supported attributes*/
    if (UNC_VF_NOT_SUPPORTED == ffe_val->valid[loop]) {
      ffe_val->cs_attr[loop] = UNC_CS_NOT_SUPPORTED;
    } else if ((UNC_VF_VALID == ffe_val->valid[loop])
               || (UNC_VF_VALID_NO_VALUE == ffe_val->valid[loop])) {
      ffe_val->cs_attr[loop] = cs_status;
    } else if ((UNC_VF_INVALID == ffe_val->valid[loop]) &&
               (UNC_OP_CREATE == op)) {
      ffe_val->cs_attr[loop] = UNC_CS_NOT_APPLIED;
    } else if ((UNC_VF_INVALID == ffe_val->valid[loop]) &&
               (UNC_OP_UPDATE == op)) {
      if (val_main->valid[loop] == UNC_VF_VALID) {
        if (cs_status == UNC_CS_APPLIED) {
          ffe_val->cs_attr[loop] = cs_status;
        }
      }
    } else if ((UNC_VF_VALID == ffe_val->valid[loop]) &&
               (UNC_OP_UPDATE == op)) {
      if (cs_status == UNC_CS_APPLIED) {
        ffe_val->cs_attr[loop] = UNC_CS_APPLIED;
      }
    }
    if ((ffe_val->valid[loop] == UNC_VF_VALID_NO_VALUE)
        &&(UNC_OP_UPDATE == op)) {
      ffe_val->cs_attr[loop]  = UNC_CS_UNKNOWN;
    }
  }
  DELETE_IF_NOT_NULL(vrt_ffe_run_key);
  return result_code;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::SetRenameFlag(ConfigKeyVal *ikey,
                                                   DalDmlIntf *dmi,
                                                   IpcReqRespHeader *req) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  val_flowfilter_entry_t *val_ffe = reinterpret_cast
      <val_flowfilter_entry_t *>(GetVal(ikey));
  if (!val_ffe) {
    UPLL_LOG_DEBUG("Val is NULL");
    return UPLL_RC_ERR_GENERIC;
  }
  ConfigKeyVal *pkey = NULL;
  if (UNC_OP_CREATE == req->operation) {
    result_code = GetParentConfigKey(pkey, ikey);
    if (UPLL_RC_SUCCESS != result_code) {
      UPLL_LOG_DEBUG("GetParentConfigKey failed %d", result_code);
      return result_code;
    }
    MoMgrImpl *mgr =
        reinterpret_cast<MoMgrImpl *>(const_cast<MoManager *>(GetMoManager(
                    UNC_KT_VRTIF_FLOWFILTER)));
    if (!mgr) {
      UPLL_LOG_DEBUG("mgr is NULL");
      DELETE_IF_NOT_NULL(pkey);
      return UPLL_RC_ERR_GENERIC;
    }
    uint8_t rename = 0;
    result_code = mgr->IsRenamed(pkey, req->datatype, dmi, rename);
    if (UPLL_RC_SUCCESS != result_code) {
      UPLL_LOG_DEBUG("IsRenamed failed %d", result_code);
      DELETE_IF_NOT_NULL(pkey);
      return result_code;
    }
    UPLL_LOG_DEBUG("Flag from parent : %d", rename);
    DELETE_IF_NOT_NULL(pkey);
    //  Check flowlist is renamed
    if ((UNC_VF_VALID == val_ffe->valid[UPLL_IDX_FLOWLIST_NAME_FFE]) &&
        ((UNC_OP_CREATE == req->operation))) {
      ConfigKeyVal *fl_ckv = NULL;
      result_code = GetFlowlistConfigKey(reinterpret_cast<const char *>
                                         (val_ffe->flowlist_name), fl_ckv, dmi);
      if (UPLL_RC_SUCCESS != result_code) {
        UPLL_LOG_DEBUG("GetFlowlistConfigKey failed %d", result_code);
        return result_code;
      }
      MoMgrImpl *fl_mgr =
          reinterpret_cast<MoMgrImpl *>(const_cast<MoManager *>(GetMoManager(
                      UNC_KT_FLOWLIST)));
      if (NULL == fl_mgr) {
        UPLL_LOG_DEBUG("fl_mgr is NULL");
        DELETE_IF_NOT_NULL(fl_ckv);
        return UPLL_RC_ERR_GENERIC;
      }
      uint8_t fl_rename = 0;
      result_code = fl_mgr->IsRenamed(fl_ckv, req->datatype, dmi, fl_rename);
      if (UPLL_RC_SUCCESS != result_code) {
        UPLL_LOG_DEBUG("IsRenamed failed %d", result_code);
        DELETE_IF_NOT_NULL(fl_ckv);
        return result_code;
      }
      if (fl_rename & 0x01) {
        rename |= FLOW_RENAME;  // TODO(upll): Check for correct flag value
      }
      DELETE_IF_NOT_NULL(fl_ckv);
    }
    SET_USER_DATA_FLAGS(ikey, rename);
  } else if (UNC_OP_UPDATE == req->operation) {
    uint8_t rename = 0;
    ConfigKeyVal *dup_ckv = NULL;
    result_code = GetChildConfigKey(dup_ckv, ikey);
    if (UPLL_RC_SUCCESS != result_code) {
      UPLL_LOG_DEBUG(" GetChildConfigKey failed");
      return result_code;
    }
    DbSubOp dbop1 = {kOpReadSingle, kOpMatchNone, kOpInOutFlag};
    result_code = ReadConfigDB(dup_ckv, req->datatype, UNC_OP_READ,
                               dbop1, dmi, MAINTBL);
    if (UPLL_RC_SUCCESS != result_code) {
      UPLL_LOG_DEBUG("ReadConfigDB failed %d", result_code);
      DELETE_IF_NOT_NULL(dup_ckv);
      return result_code;
    }
    GET_USER_DATA_FLAGS(dup_ckv, rename);
    DELETE_IF_NOT_NULL(dup_ckv);
    if (UNC_VF_VALID == val_ffe->valid[UPLL_IDX_FLOWLIST_NAME_FFE]) {
      ConfigKeyVal *fl_ckv = NULL;
      result_code = GetFlowlistConfigKey(reinterpret_cast<const char *>
                                         (val_ffe->flowlist_name), fl_ckv, dmi);
      if (UPLL_RC_SUCCESS != result_code) {
        UPLL_LOG_DEBUG("GetFlowlistConfigKey failed %d", result_code);
        return result_code;
      }
      MoMgrImpl *fl_mgr =
          reinterpret_cast<MoMgrImpl *>(const_cast<MoManager *>(GetMoManager(
                      UNC_KT_FLOWLIST)));
      if (NULL == fl_mgr) {
        UPLL_LOG_DEBUG("fl_mgr is NULL");
        DELETE_IF_NOT_NULL(fl_ckv);
        return UPLL_RC_ERR_GENERIC;
      }
      uint8_t fl_rename = 0;
      result_code = fl_mgr->IsRenamed(fl_ckv, req->datatype, dmi, fl_rename);
      if (UPLL_RC_SUCCESS != result_code) {
        UPLL_LOG_DEBUG("IsRenamed failed %d", result_code);
        DELETE_IF_NOT_NULL(fl_ckv);
        return result_code;
      }
      if (fl_rename & 0x01) {
        rename |= FLOW_RENAME;  // TODO(upll): Check for correct flag value
      } else {
        rename &= NO_FLOWLIST_RENAME;
        /* reset flag*/
      }
      DELETE_IF_NOT_NULL(fl_ckv);
    } else if (UNC_VF_VALID_NO_VALUE == val_ffe->valid
               [UPLL_IDX_FLOWLIST_NAME_FFE]) {
      rename &= NO_FLOWLIST_RENAME;  // TODO(upll): Check for correct
      // flag value.
      //  No rename flowlist value should be set
    }
    SET_USER_DATA_FLAGS(ikey, rename);
  }
  return UPLL_RC_SUCCESS;
}

upll_rc_t VrtIfFlowFilterEntryMoMgr::GetFlowlistConfigKey(
    const char *flowlist_name, ConfigKeyVal *&okey,
    DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_ERR_GENERIC;
  MoMgrImpl *mgr =
      reinterpret_cast<MoMgrImpl *>(const_cast<MoManager *>(GetMoManager(
                  UNC_KT_FLOWLIST)));
  result_code = mgr->GetChildConfigKey(okey, NULL);
  if (UPLL_RC_SUCCESS != result_code) {
    UPLL_LOG_DEBUG("GetChildConfigKey failed %d", result_code);
    return result_code;
  }
  key_flowlist_t *okey_key = reinterpret_cast<key_flowlist_t *>
      (okey->get_key());
  uuu::upll_strncpy(okey_key->flowlist_name,
                    flowlist_name,
                    (kMaxLenFlowListName+1));
  return UPLL_RC_SUCCESS;
}
}  // namespace kt_momgr
}  // namespace upll
}  // namespace unc
