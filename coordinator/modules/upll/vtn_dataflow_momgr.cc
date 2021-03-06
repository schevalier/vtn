/*
 * Copyright (c) 2013-2014 NEC Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#include "vbr_if_momgr.hh"
#include "vtn_momgr.hh"
#include "vnode_child_momgr.hh"
#include "ipc_client_handler.hh"

#include "vtn_dataflow_momgr.hh"

using unc::upll::ipc_util::IpcUtil;
using unc::upll::ipc_util::IpcClientHandler;

namespace unc {
namespace upll {
namespace kt_momgr {


upll_rc_t VtnDataflowMoMgr::ValidateMessage(IpcReqRespHeader *req,
                                            ConfigKeyVal *ikey) {
  UPLL_FUNC_TRACE;
  if (!req || !ikey || !(ikey->get_key())) {
    UPLL_LOG_INFO("ConfigKeyVal / IpcReqRespHeader is Null");
    return UPLL_RC_ERR_BAD_REQUEST;
  }
  if (req->operation != UNC_OP_READ) {
    UPLL_LOG_INFO("Unsupported Operation for VTN Dataflow - %d",
                  req->operation);
    return UPLL_RC_ERR_NOT_ALLOWED_FOR_THIS_KT;
  }
  if (req->datatype != UPLL_DT_STATE) {
    UPLL_LOG_INFO("Unsupported datatype for VTN Dataflow - %d",
                  req->datatype);
    return UPLL_RC_ERR_NOT_ALLOWED_FOR_THIS_DT;
  }
  if (req->option1 != UNC_OPT1_NORMAL) {
    UPLL_LOG_INFO("Invalid option1 for VTN Dataflow - %d", req->option1);
    return UPLL_RC_ERR_INVALID_OPTION1;
  }
  if (req->option2 != UNC_OPT2_NONE) {
    UPLL_LOG_INFO("Invalid option2 for VTN Dataflow - %d", req->option2);
    return UPLL_RC_ERR_INVALID_OPTION2;
  }
  if (UNC_KT_VTN_DATAFLOW != ikey->get_key_type()) {
    UPLL_LOG_INFO("Invalid KeyType in the request (%d)",
                  ikey->get_key_type());
    return UPLL_RC_ERR_BAD_REQUEST;
  }
  if (ikey->get_st_num() != IpctSt::kIpcStKeyVtnDataflow) {
    UPLL_LOG_INFO("Invalid Key structure received. received struct - %d",
                  (ikey->get_st_num()));
    return UPLL_RC_ERR_BAD_REQUEST;
  }
  if (ikey->get_cfg_val()) {
    UPLL_LOG_INFO("Value structure is not NULL for VTN Dataflow");
    return UPLL_RC_ERR_BAD_REQUEST;
  }
  key_vtn_dataflow *vtn_df_key = reinterpret_cast<key_vtn_dataflow *>
      (ikey->get_key());
  return(ValidateVtnDataflowKey(vtn_df_key));
}

upll_rc_t VtnDataflowMoMgr::ValidateVtnDataflowKey(
    key_vtn_dataflow *vtn_df_key) {
  UPLL_FUNC_TRACE;
  upll_rc_t ret_val = UPLL_RC_SUCCESS;
  ret_val = ValidateKey(reinterpret_cast<char *>(vtn_df_key->vtn_key.vtn_name),
                        kMinLenVtnName, kMaxLenVtnName);
  if (ret_val != UPLL_RC_SUCCESS) {
    UPLL_LOG_INFO("vtn name syntax check failed for Vtn Dataflow."
                  "Received vtn name - %s",
                  vtn_df_key->vtn_key.vtn_name);
    return ret_val;
  }
  ret_val = ValidateKey(reinterpret_cast<char *>(vtn_df_key->vnode_id),
                        kMinLenVnodeName, kMaxLenVnodeName);
  if (ret_val != UPLL_RC_SUCCESS) {
    UPLL_LOG_INFO("vnode name syntax check failed for Vtn Dataflow."
                  "Received vnode name - %s",
                  vtn_df_key->vnode_id);
    return ret_val;
  }
  if ((vtn_df_key->vlanid != 0xFFFF) &&
      !ValidateNumericRange(vtn_df_key->vlanid,
                            kMinVlanId, kMaxVlanId,
                            true, true)) {
    UPLL_LOG_INFO("Vlan Id Number check failed for Vtn Dataflow."
                  "Received vlan_id - %d",
                  vtn_df_key->vlanid);
    return UPLL_RC_ERR_CFG_SYNTAX;
  }
  // TODO(rev): What kind of MAC validation need to be done for VTN Dataflow?
  if (!ValidateMacAddr(vtn_df_key->src_mac_address)) {
    UPLL_LOG_INFO("Mac Address validation failure for Vtn Dataflow."
                  " Received  mac_address is - %s",
                  vtn_df_key->src_mac_address);
    return UPLL_RC_ERR_CFG_SYNTAX;
  }
  return UPLL_RC_SUCCESS;
}

upll_rc_t VtnDataflowMoMgr::ValidateControllerCapability(
    const char *ctrlr_name, bool is_first_ctrlr,
    unc_keytype_ctrtype_t *ctrlr_type) {
  UPLL_FUNC_TRACE;

  if (!is_first_ctrlr) {
    if (!uuc::CtrlrMgr::GetInstance()->GetCtrlrType(
            ctrlr_name, UPLL_DT_RUNNING, ctrlr_type)) {
      UPLL_LOG_INFO("GetCtrlrType failed for ctrlr %s", ctrlr_name);
      return UPLL_RC_ERR_GENERIC;
    }
    UPLL_LOG_TRACE("Controller type is  %d", *ctrlr_type);
    if (*ctrlr_type != UNC_CT_PFC) {
      return UPLL_RC_SUCCESS;
    }
  }

  uint32_t max_attrs = 0;
  const uint8_t *attrs = NULL;
  if (!GetReadCapability(ctrlr_name, UNC_KT_VTN_DATAFLOW,
                         &max_attrs, &attrs, UPLL_DT_RUNNING)) {
    UPLL_LOG_DEBUG("Read vtn_dataflow is not supported by controller %s",
                   ctrlr_name);
    return UPLL_RC_ERR_NOT_SUPPORTED_BY_CTRLR;
  }

  return UPLL_RC_SUCCESS;
}

upll_rc_t VtnDataflowMoMgr::FillCtrlrDomCountMap(uint8_t *vtn_name,
                                                 uint32_t  &ctrlr_dom_count,
                                                 DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  ConfigKeyVal *tmp_ckv = NULL;
  VtnMoMgr *vtnmgr = static_cast<VtnMoMgr *>((const_cast<MoManager *>
                                              (GetMoManager(UNC_KT_VTN))));
  result_code = vtnmgr->GetChildConfigKey(tmp_ckv, NULL);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_INFO("GetChildConfigKey failed result_code %d", result_code);
    return result_code;
  }
  uuu::upll_strncpy(reinterpret_cast<key_vtn *>(
          tmp_ckv->get_key())->vtn_name,
      vtn_name, (kMaxLenVtnName + 1));
  result_code = vtnmgr->GetInstanceCount(tmp_ckv, NULL,
                                         UPLL_DT_RUNNING,
                                         &ctrlr_dom_count,
                                         dmi,
                                         CTRLRTBL);
  if (result_code != UPLL_RC_SUCCESS) {
    delete tmp_ckv;
    return result_code;
  }
  delete tmp_ckv;
  return result_code;
}

upll_rc_t VtnDataflowMoMgr::ConvertVexternaltoVbr(const uint8_t *vtn_name,
                                                  uint8_t *vex_name,
                                                  uint8_t *vex_if_name,
                                                  DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  MoMgrImpl *vbrif_mgr = static_cast<MoMgrImpl *>(
      (const_cast<MoManager*>
       (GetMoManager(UNC_KT_VBR_IF))));
  if (!vbrif_mgr) {
    UPLL_LOG_DEBUG("Instance is NULL");
    return UPLL_RC_ERR_GENERIC;
  }
  ConfigKeyVal *ckv_if = NULL;
  result_code = vbrif_mgr->GetChildConfigKey(ckv_if, NULL);
  if (UPLL_RC_SUCCESS != result_code) {
    UPLL_LOG_DEBUG("GetChildConfigKey Failed");
    return result_code;
  }
  key_vbr_if *if_key = reinterpret_cast<key_vbr_if *>(ckv_if->get_key());
  val_drv_vbr_if *if_val = reinterpret_cast<val_drv_vbr_if *>
      (ConfigKeyVal::
       Malloc(sizeof(val_drv_vbr_if)));
  if_val->valid[PFCDRV_IDX_VEXT_NAME_VBRIF] = UNC_VF_VALID;
  ckv_if->SetCfgVal(new ConfigVal(IpctSt::kIpcStPfcdrvValVbrIf, if_val));
  uuu::upll_strncpy(if_key->vbr_key.vtn_key.vtn_name, vtn_name,
                    (kMaxLenVtnName + 1));
  uuu::upll_strncpy(if_val->vex_name, vex_name, (kMaxLenVnodeName+1));
  DbSubOp dbop = { kOpReadSingle, kOpMatchNone, kOpInOutFlag };
  result_code = vbrif_mgr->ReadConfigDB(ckv_if, UPLL_DT_RUNNING, UNC_OP_READ,
                                        dbop, dmi, MAINTBL);
  if (UPLL_RC_SUCCESS != result_code &&
      UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code) {
    delete ckv_if;
    return result_code;
  }
  if (UPLL_RC_SUCCESS == result_code) {
    uuu::upll_strncpy(vex_name, if_key->vbr_key.vbridge_name,
                      (kMaxLenVnodeName+1));
    uuu::upll_strncpy(vex_if_name, if_key->if_name, (kMaxLenInterfaceName+1));
  }
  delete ckv_if;
  return UPLL_RC_SUCCESS;
}

upll_rc_t VtnDataflowMoMgr::MapCtrlrNameToUncName(
    const uint8_t *vtn_name,
    val_vtn_dataflow_path_info *path_info,
    uint8_t *ctrlr_id,
    DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  ConfigKeyVal *ckv_vn = NULL;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  if (!path_info) {
    return UPLL_RC_ERR_GENERIC;
  }
  MoMgrImpl *vbr_mgr = static_cast<MoMgrImpl *>(
      (const_cast<MoManager *>
       (GetMoManager(UNC_KT_VBRIDGE))));
  result_code = vbr_mgr->GetChildConfigKey(ckv_vn, NULL);
  if (UPLL_RC_SUCCESS != result_code) {
    UPLL_LOG_DEBUG("GetChildConfigKey Failed");
    return result_code;
  }
  key_vbr_t *vbr_key = reinterpret_cast<key_vbr_t*>(ckv_vn->get_key());

  uuu::upll_strncpy(vbr_key->vtn_key.vtn_name,
                    vtn_name, (kMaxLenVtnName + 1));

  for (int iter = 0 ; iter < 2 ; iter++) {
    uint32_t indx = (iter == 0)?UPLL_IDX_IN_VNODE_VVDPI:
        UPLL_IDX_OUT_VNODE_VVDPI;
    uint32_t if_indx = (iter == 0)?UPLL_IDX_IN_VIF_VVDPI:
        UPLL_IDX_OUT_VIF_VVDPI;
    uint8_t *node_name =(iter == 0)?path_info->in_vnode:path_info->out_vnode;
    uint8_t *node_if_name =(iter == 0)?path_info->in_vif:path_info->out_vif;

    if (path_info->valid[indx] == UNC_VF_INVALID ||
        path_info->valid[if_indx] == UNC_VF_INVALID) {
      UPLL_LOG_DEBUG("Vnode or Vnode interface is invalid in path"
                     "info %d", iter);
      continue;
    }
    uuu::upll_strncpy(vbr_key->vbridge_name,
                      node_name, (kMaxLenVnodeName + 1));
    result_code = vbr_mgr->GetRenamedUncKey(ckv_vn, UPLL_DT_RUNNING, dmi,
                                            ctrlr_id);
    if (UPLL_RC_ERR_NO_SUCH_INSTANCE == result_code) {
      // search in vbr_if table by node name in vex
      result_code = UPLL_RC_SUCCESS;
      if (!path_info->vlink_flag &&
          path_info->valid[UPLL_IDX_VLINK_FLAG_VVDPI] == UNC_VF_VALID) {
        /*
         * Converting redirect vexternal to vbr and interface
         */
        result_code = ConvertVexternaltoVbr(vtn_name,
                                            node_name,
                                            node_if_name, dmi);
        if (UPLL_RC_SUCCESS != result_code) {
          UPLL_LOG_DEBUG("ConvertVexternalToVbr Failed %d", result_code);
          delete ckv_vn;
          return result_code;
        }
      }

    } else if (UPLL_RC_SUCCESS != result_code) {
      UPLL_LOG_DEBUG("GetRenamedUncKey Failed %d", result_code);
      delete ckv_vn;
      return result_code;
    } else {
      if (path_info->valid[indx] == UNC_VF_VALID) {
        uuu::upll_strncpy(node_name, vbr_key->vbridge_name,
                          (kMaxLenVnodeName + 1));
      }
    }
  }
  delete ckv_vn;
  return result_code;
}

upll_rc_t VtnDataflowMoMgr::UpdatePathInfoInterfaces(
    DataflowCmn *df_cmn,
    const uint8_t *vtn_name,
    DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  uint32_t path_info_count;
  // NULL check for df_cmn/df_segment done in calling functino
  if ((path_info_count =
       df_cmn->df_segment->vtn_df_common->path_info_count) == 0) {
    UPLL_LOG_TRACE("Path count is zero");
    return UPLL_RC_SUCCESS;
  }
  if (path_info_count != df_cmn->df_segment->vtn_path_infos.size()) {
    UPLL_LOG_INFO("Path info size not consistent vtn_cmn path count = %d "
                  "and path_info size = %" PFC_PFMT_SIZE_T "",
                  path_info_count, df_cmn->df_segment->vtn_path_infos.size());
    return UPLL_RC_ERR_GENERIC;
  }
  if (!vtn_name) {
    UPLL_LOG_INFO("Invalid vtn name \n");
    return UPLL_RC_ERR_GENERIC;
  }
  // Convert ingress and egress vex/vexif if vlink_flag is reset (redirected).
  val_vtn_dataflow_path_info *path_info =
      df_cmn->df_segment->vtn_path_infos[0];
  uint32_t node_indx = UPLL_IDX_IN_VNODE_VVDPI;
  uint32_t nodeif_indx = UPLL_IDX_IN_VIF_VVDPI;
  bool dynamic = false;
  int iter = 0;
  // convert/rename first and last path_info
  while (path_info) {
    dynamic = false;
    UPLL_LOG_TRACE("ConvertVexternaltoVbr ");
    UPLL_LOG_TRACE("node:%s",
                   DataflowCmn::get_string(*path_info).c_str());
    uint8_t *node_name =(iter == 0)?path_info->in_vnode:path_info->out_vnode;
    uint8_t *node_if_name =(iter == 0)?path_info->in_vif:path_info->out_vif;
    uint8_t *df_node_name =(iter == 0)?
        df_cmn->df_segment->vtn_df_common->ingress_vnode:
        df_cmn->df_segment->vtn_df_common->egress_vnode;
    uint8_t *df_node_if_name =(iter == 0)?
        df_cmn->df_segment->vtn_df_common->ingress_vinterface:
        df_cmn->df_segment->vtn_df_common->egress_vinterface;
    UPLL_LOG_TRACE("vlink flag and dynamic %d:%d", path_info->vlink_flag,
                   dynamic);
    if ((path_info->valid[node_indx] == UNC_VF_VALID) &&
        (path_info->valid[nodeif_indx] == UNC_VF_VALID)) {
      // if the vnode name has not been translated, it is a dynamic interface.
      if (!strcmp(reinterpret_cast<const char *>(df_node_name),
                  reinterpret_cast<const char *>(node_name))) {
        dynamic = true;
      }
      if (!dynamic && !path_info->vlink_flag) {
        uuu::upll_strncpy(node_name, df_node_name,
                          (kMaxLenVnodeName+1));
        uuu::upll_strncpy(node_if_name, df_node_if_name,
                          (kMaxLenInterfaceName+1));
      } else {
        if ((!iter) ||
            ((df_cmn->df_segment->vtn_df_common->
              valid[UPLL_IDX_EGRESS_VNODE_VVDC]
              == UNC_VF_VALID && df_cmn->df_segment->vtn_df_common->
              valid[UPLL_IDX_EGRESS_VINTERFACE_VVDC] == UNC_VF_VALID))) {
          path_info->valid[node_indx] = UNC_VF_INVALID;
          path_info->valid[nodeif_indx] = UNC_VF_INVALID;
        }
      }
    }
    if ((node_indx == UPLL_IDX_OUT_VNODE_VVDPI) ||
        (path_info_count == 1)) {
      path_info = NULL;
    } else {
      node_indx = UPLL_IDX_OUT_VNODE_VVDPI;
      nodeif_indx = UPLL_IDX_OUT_VIF_VVDPI;
      path_info =  df_cmn->df_segment->vtn_path_infos[path_info_count - 1];
    }
    iter++;
  }

  // rename intermediate nodes (they can be vexternal too)
  for (unsigned int iter = 1;
       iter < (df_cmn->df_segment->vtn_path_infos.size()-1); iter++) {
    path_info = df_cmn->df_segment->vtn_path_infos[iter];
    result_code = MapCtrlrNameToUncName(
        vtn_name,
        path_info,
        df_cmn->df_segment->vtn_df_common->controller_id,
        dmi);
    if (UPLL_RC_SUCCESS != result_code) {
      UPLL_LOG_DEBUG("MapCtrlrNameToUncName failed %d", result_code);
      return result_code;
    }
  }
  return UPLL_RC_SUCCESS;
}


upll_rc_t VtnDataflowMoMgr::MapVexternalToVbridge(
    const ConfigKeyVal *ckv_df,
    DataflowCmn *df_cmn,
    bool *is_vnode_match,
    DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  const key_vtn_dataflow_t *vtn_df_key = reinterpret_cast
      <const key_vtn_dataflow_t *>
      (ckv_df->get_key());
  if (!vtn_df_key || !df_cmn) {
    UPLL_LOG_DEBUG("Input key is NULL");
    return UPLL_RC_ERR_GENERIC;
  }
  controller_domain ctrlr_dom = {NULL, NULL};
  GET_USER_DATA_CTRLR_DOMAIN(ckv_df, ctrlr_dom);
  UPLL_LOG_TRACE("In and Out Domain %s", ctrlr_dom.domain);
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  VbrIfMoMgr *vbrif_mgr = static_cast<VbrIfMoMgr *>(
      (const_cast<MoManager *>
       (GetMoManager(UNC_KT_VBR_IF))));
  if (!vbrif_mgr) {
    UPLL_LOG_DEBUG("Instance is NULL");
    return UPLL_RC_ERR_GENERIC;
  }
  if (df_cmn->df_segment->flow_traversed == 0) {
    if (ctrlr_dom.domain &&
        strlen(reinterpret_cast<const char*>(ctrlr_dom.domain))) {
      uuu::upll_strncpy(df_cmn->df_segment->vtn_df_common->ingress_domain,
                        ctrlr_dom.domain, kMaxLenDomainId+1);
      df_cmn->df_segment->vtn_df_common->valid[UPLL_IDX_INGRESS_DOMAIN_VVDC] =
          UNC_VF_VALID;
      if (df_cmn->df_segment->vtn_df_common->
          valid[UPLL_IDX_EGRESS_VINTERFACE_VVDC]
          == UNC_VF_VALID) {
        uuu::upll_strncpy(df_cmn->df_segment->vtn_df_common->egress_domain,
                          ctrlr_dom.domain, kMaxLenDomainId+1);
        df_cmn->df_segment->vtn_df_common->valid[UPLL_IDX_EGRESS_DOMAIN_VVDC] =
            UNC_VF_VALID;
      }
    }
    const uint8_t *vtn_name = vtn_df_key->vtn_key.vtn_name;
    UPLL_LOG_TRACE("node:%s",
                   DataflowCmn::get_string(
                       *df_cmn->df_segment->vtn_df_common).c_str());
    bool dynamic[2] = {false, false };
    ConfigKeyVal *ckv_vbrif[2] = {NULL, NULL};
    // Do the loop twice, once for ingress and once for egress
    for (int iter = 0; iter < 2 ; ++iter) {
      uint8_t *vnode = NULL, *vnode_if = NULL;
      uint8_t valid[2];
      key_vbr_if_t *key_vbrif;
      if (iter == 0) {
        valid[0] =
            df_cmn->df_segment->vtn_df_common->
            valid[UPLL_IDX_INGRESS_VNODE_VVDC];
        valid[1] = df_cmn->df_segment->
            vtn_df_common->valid[UPLL_IDX_INGRESS_VINTERFACE_VVDC];
        vnode = df_cmn->df_segment->vtn_df_common->ingress_vnode;
        vnode_if = df_cmn->df_segment->vtn_df_common->ingress_vinterface;
      } else {
        valid[0] =
            df_cmn->df_segment->vtn_df_common->
            valid[UPLL_IDX_EGRESS_VNODE_VVDC];
        valid[1] = df_cmn->df_segment->
            vtn_df_common->valid[UPLL_IDX_EGRESS_VINTERFACE_VVDC];
        vnode = df_cmn->df_segment->vtn_df_common->egress_vnode;
        vnode_if = df_cmn->df_segment->vtn_df_common->egress_vinterface;
      }
      if (valid[0] != UNC_VF_VALID && valid[1] != UNC_VF_VALID) {
        UPLL_LOG_INFO("Ingress/Egress vNode/vInterface is not valid");
        result_code =  UPLL_RC_ERR_NO_SUCH_INSTANCE;
        break;
      } else {
        result_code = vbrif_mgr->GetChildConfigKey(ckv_vbrif[iter], NULL);
        if (UPLL_RC_SUCCESS != result_code) {
          UPLL_LOG_DEBUG("GetChildConfigKey Failed");
          return result_code;
        }
        key_vbrif = static_cast<key_vbr_if_t *>(ckv_vbrif[iter]->get_key());
        val_drv_vbr_if_t *drv_val_vbrif = static_cast<val_drv_vbr_if_t *>
            (ConfigKeyVal::Malloc(sizeof(val_drv_vbr_if_t)));
        ckv_vbrif[iter]->SetCfgVal(
            new ConfigVal(IpctSt::kIpcStPfcdrvValVbrIf, drv_val_vbrif));
        uuu::upll_strncpy(key_vbrif->vbr_key.vtn_key.vtn_name,
                          vtn_df_key->vtn_key.vtn_name, (kMaxLenVtnName + 1));
        uuu::upll_strncpy(drv_val_vbrif->vex_name,
                          vnode, (kMaxLenVnodeName + 1));
        uuu::upll_strncpy(drv_val_vbrif->vex_if_name,
                          vnode_if, (kMaxLenInterfaceName + 1));
        drv_val_vbrif->valid[PFCDRV_IDX_VEXT_NAME_VBRIF] = UNC_VF_VALID;
        drv_val_vbrif->valid[PFCDRV_IDX_VEXT_IF_NAME_VBRIF] = UNC_VF_VALID;
        DbSubOp dbop = { kOpReadSingle,
          kOpMatchNone,
          kOpInOutFlag | kOpInOutCtrlr |
          kOpInOutDomain };
        result_code = vbrif_mgr->ReadConfigDB(ckv_vbrif[iter], UPLL_DT_RUNNING,
                                              UNC_OP_READ, dbop, dmi, MAINTBL);
      }
      if (result_code == UPLL_RC_ERR_NO_SUCH_INSTANCE) {
        UPLL_LOG_INFO("Dynamic interface result_code"
                      " %d", result_code);
        dynamic[iter] = true;
        result_code = UPLL_RC_SUCCESS;
      } else if (result_code != UPLL_RC_SUCCESS) {
        UPLL_LOG_INFO("vbrif ReadConfigDB Failed err code - %d",
                      result_code);
        DELETE_IF_NOT_NULL(ckv_vbrif[0]);
        DELETE_IF_NOT_NULL(ckv_vbrif[1]);
        return result_code;
      } else {
        key_vbrif = reinterpret_cast<key_vbr_if_t *>
            (ckv_vbrif[iter]->get_key());
        uuu::upll_strncpy(vnode, key_vbrif->vbr_key.vbridge_name,
                          (kMaxLenVnodeName + 1));
        uuu::upll_strncpy(vnode_if, key_vbrif->if_name,
                          (kMaxLenInterfaceName + 1));
      }
      if (iter == 1)
        df_cmn->df_segment->ckv_egress = ckv_vbrif[iter];
    }
    upll_rc_t upd_path_result_code = UpdatePathInfoInterfaces(df_cmn,
                                                              vtn_name,
                                                              dmi);
    if (UPLL_RC_SUCCESS != upd_path_result_code) {
      UPLL_LOG_TRACE("UpdatePathInfoInterface Failed %d", result_code);
      DELETE_IF_NOT_NULL(ckv_vbrif[0]);
      DELETE_IF_NOT_NULL(ckv_vbrif[1]);
      result_code = upd_path_result_code;
    }
    if (dynamic[0]) {
      UPLL_LOG_DEBUG("Ingress interface is dynamic");
      val_vtn_dataflow_path_info *path_info =
          df_cmn->df_segment->vtn_path_infos[0];
      path_info->valid[UPLL_IDX_IN_VIF_VVDPI] = UNC_VF_INVALID;
      path_info->valid[UPLL_IDX_OUT_VIF_VVDPI] = UNC_VF_INVALID;
      df_cmn->df_segment->vtn_df_common->
          valid[UPLL_IDX_INGRESS_VINTERFACE_VVDC]
          = UNC_VF_INVALID;
      if (UNC_VF_VALID == path_info->valid[UPLL_IDX_OUT_VNODE_VVDPI]) {
        uuu::upll_strncpy(df_cmn->df_segment->vtn_df_common->ingress_vnode,
                          path_info->out_vnode, kMaxLenVnodeName+1);
      }
    }
    if (dynamic[1]) {
      UPLL_LOG_DEBUG("Egress interface is dynamic");
      val_vtn_dataflow_path_info *last_path_info =
          (df_cmn->df_segment->vtn_path_infos).back();
      last_path_info->valid[UPLL_IDX_IN_VIF_VVDPI] = UNC_VF_INVALID;
      last_path_info->valid[UPLL_IDX_OUT_VIF_VVDPI] = UNC_VF_INVALID;
      df_cmn->df_segment->vtn_df_common->valid[UPLL_IDX_EGRESS_VINTERFACE_VVDC]
          = UNC_VF_INVALID;
      if (UNC_VF_VALID == last_path_info->valid[UPLL_IDX_IN_VNODE_VVDPI]) {
        uuu::upll_strncpy(df_cmn->df_segment->vtn_df_common->egress_vnode,
                          last_path_info->in_vnode, kMaxLenVnodeName+1);
      }
    }
    DELETE_IF_NOT_NULL(ckv_vbrif[0]);
    df_cmn->df_segment->flow_traversed++;
  }
  key_vtn_dataflow *key_df = reinterpret_cast
      <key_vtn_dataflow*>(ckv_df->get_key());
  *is_vnode_match = (0 ==
                     strcmp(reinterpret_cast<const char *>
                            (df_cmn->df_segment->vtn_df_common->ingress_vnode),
                            reinterpret_cast<const char *>(key_df->vnode_id)));
  // If the given dataflow ingress vnode does not match the given vnode,
  //       // we need to filter the dataflow.
  UPLL_LOG_TRACE(" The Status of is_vnode_match %d", *is_vnode_match);
  if (!(*is_vnode_match)) {
    return UPLL_RC_SUCCESS;
  }
  return result_code;
}

upll_rc_t VtnDataflowMoMgr::PopulateVnpOrVbypassBoundaryInfo(
    ConfigKeyVal *&ckv_inif,
    ConfigKeyVal *&ckv_egress,
    DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  MoMgrImpl *mgr = reinterpret_cast<MoMgrImpl *>(
      const_cast<MoManager *>(GetMoManager(ckv_inif->get_key_type())));
  if (!mgr) {
    UPLL_LOG_INFO("Invalid mgr param");
    return UPLL_RC_ERR_GENERIC;
  }
  result_code = mgr->GetChildConfigKey(ckv_egress, ckv_inif);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_INFO("Error in retrieving the Child ConfigKeyVal");
    return result_code;
  }
  uint8_t *ifname = reinterpret_cast <key_vnode_if *>
      (ckv_egress->get_key())->vnode_if_name;
  memset(ifname, 0, kMaxLenInterfaceName);
  DbSubOp dbop = {kOpReadMultiple, kOpMatchNone, kOpInOutCtrlr |
    kOpInOutDomain | kOpInOutFlag};
  /* Get the list interfaces for given parent keytype */
  result_code = mgr->ReadConfigDB(ckv_egress, UPLL_DT_RUNNING, UNC_OP_READ,
                                  dbop, dmi, MAINTBL);
  /* Any other DB error */
  if (UPLL_RC_SUCCESS != result_code) {
    UPLL_LOG_INFO("ReadConfigDB failed %d", result_code);
    DELETE_IF_NOT_NULL(ckv_egress);
    return result_code;
  }
  return result_code;
}


upll_rc_t
VtnDataflowMoMgr::ReadMo(IpcReqRespHeader *header,
                         ConfigKeyVal *ckv_in,
                         DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  DataflowUtil df_util;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  result_code = ValidateMessage(header, ckv_in);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_INFO("ValidateMessage failed result_code %d",
                  result_code);
    return result_code;
  }
  pfc::core::ipc::ServerSession *sess = reinterpret_cast
      <pfc::core::ipc::ServerSession *>(ckv_in->get_user_data());
  if (!sess) {
    UPLL_LOG_INFO("Empty session");
    return UPLL_RC_ERR_GENERIC;
  }
  ConfigKeyVal *ckv_req = NULL;
  result_code = GetChildConfigKey(ckv_req, ckv_in);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_INFO("GetChildConfigKey failed result_code %d",
                  result_code);
    return result_code;
  }
  key_vtn_dataflow_t *vtn_df_key = reinterpret_cast<key_vtn_dataflow_t *>
      (ckv_req->get_key());
  ConfigKeyVal *ckv_vbr = NULL;
  VnodeMoMgr *vnmgr = static_cast<VnodeMoMgr *>(
      (const_cast<MoManager *>(GetMoManager(UNC_KT_VBRIDGE))));
  result_code = vnmgr->GetChildConfigKey(ckv_vbr, NULL);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_INFO("GetChildConfigKey failed result_code %d", result_code);
    delete ckv_req;
    return result_code;
  }
  key_vbr *vbr_key = reinterpret_cast<key_vbr *>(ckv_vbr->get_key());
  uuu::upll_strncpy(vbr_key->vtn_key.vtn_name,
                    vtn_df_key->vtn_key.vtn_name, (kMaxLenVtnName+1));
  uuu::upll_strncpy(vbr_key->vbridge_name, vtn_df_key->vnode_id,
                    (kMaxLenVnodeName+1));
  DbSubOp dbop = { kOpReadSingle, kOpMatchNone, kOpInOutCtrlr
    |kOpInOutDomain|kOpInOutFlag };
  /* Get the controller domain using this read operation */
  result_code = vnmgr->ReadConfigDB(ckv_vbr, UPLL_DT_RUNNING, UNC_OP_READ,
                                    dbop, dmi, MAINTBL);
  if (UPLL_RC_SUCCESS != result_code) {
    UPLL_LOG_INFO("ReadConfigDB failed %d", result_code);
    delete ckv_vbr;
    delete ckv_req;
    return result_code;
  }
  /* Set the controller and domain name in ckv*/
  SET_USER_DATA(ckv_req, ckv_vbr);
  delete ckv_vbr;
  unc_keytype_ctrtype_t ctrlr_type = UNC_CT_UNKNOWN;
  uint8_t *ctrlr_id = NULL;
  GET_USER_DATA_CTRLR(ckv_req, ctrlr_id);
  result_code = ValidateControllerCapability((const char *)(ctrlr_id), true,
                                             &ctrlr_type);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_INFO("ValidateControllerCapability failed result_code %d",
                  result_code);
    delete ckv_req;
    return result_code;
  }
  uint32_t ctrlr_dom_count = 0;
  result_code = FillCtrlrDomCountMap(vtn_df_key->vtn_key.vtn_name,
                                     ctrlr_dom_count, dmi);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_INFO("Unable to get max controller domain count\n");
    delete ckv_req;
    return result_code;
  }
  df_util.ctrlr_dom_count_map["nvtnctrlrdom"] = ctrlr_dom_count;
  result_code = TraversePFCController(ckv_req, header, NULL, NULL,
                                      &df_util, dmi, true);
  delete ckv_req;
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_INFO("TraversePFCController failed %d", result_code);
    return result_code;
  } else {
    if (!IpcUtil::WriteKtResponse(sess, *header, ckv_in)) {
      UPLL_LOG_INFO("Failed to send response to key tree request");
      return UPLL_RC_ERR_GENERIC;
    }
    df_util.sessOutDataflows(*sess);
  }
  return UPLL_RC_SUCCESS;
}


upll_rc_t
VtnDataflowMoMgr::TraversePFCController(ConfigKeyVal *ckv_df,
                                        IpcReqRespHeader *header,
                                        DataflowCmn *currentnode,
                                        DataflowCmn *lastPfcNode,
                                        DataflowUtil *df_util,
                                        DalDmlIntf *dmi,
                                        bool is_first_ctrlr)   {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  key_vtn_dataflow_t *vtn_df_key = reinterpret_cast<key_vtn_dataflow_t *>
      (ckv_df->get_key());
  if (!is_first_ctrlr) {
    /**
     * Reset the VlanID and Source Mac Address from the output matches
     * in the key structure while
     * retreiving flow segments
     **/

    map <UncDataflowFlowMatchType, void *>::iterator output_matches_iter;
    output_matches_iter = lastPfcNode->output_matches.find(UNC_MATCH_VLAN_ID);
    if (output_matches_iter != lastPfcNode->output_matches.end()) {
      val_df_flow_match_vlan_id_t *prev =
          reinterpret_cast<val_df_flow_match_vlan_id_t *>
          ((*output_matches_iter).second);
      vtn_df_key->vlanid =  prev->vlan_id;
    }
    output_matches_iter = lastPfcNode->output_matches.find(UNC_MATCH_DL_SRC);
    if (output_matches_iter != lastPfcNode->output_matches.end()) {
      val_df_flow_match_dl_addr_t *prev =
          reinterpret_cast<val_df_flow_match_dl_addr_t *>
          ((*output_matches_iter).second);
      memcpy(vtn_df_key->src_mac_address, prev->dl_addr,
             sizeof(vtn_df_key->src_mac_address));
    }
  }
  controller_domain ctrlr_dom = {NULL, NULL};
  GET_USER_DATA_CTRLR_DOMAIN(ckv_df, ctrlr_dom);
  if (!ctrlr_dom.ctrlr || !ctrlr_dom.domain) {
    UPLL_LOG_INFO("ctrlr_dom controller or domain is NULL");
    return UPLL_RC_ERR_GENERIC;
  }
  key_vtn_ctrlr_dataflow vtn_ctrlr_df_key(vtn_df_key,
                                          ctrlr_dom.ctrlr, ctrlr_dom.domain);
  vector<DataflowDetail *> pfc_flows;
  std::map<key_vtn_ctrlr_dataflow, vector<DataflowDetail *> >::iterator iter =
      df_util->upll_pfc_flows.begin();
  for (; iter != df_util->upll_pfc_flows.end(); iter ++) {
    if (DataflowCmn::Compare((*iter).first, vtn_ctrlr_df_key)) {
      UPLL_LOG_DEBUG("Maching the key");
      break;
    }
  }
  if (iter == df_util->upll_pfc_flows.end()) {
    ConfigKeyVal *ckv_dupdf = NULL;
    result_code = GetChildConfigKey(ckv_dupdf, ckv_df);
    if (UPLL_RC_SUCCESS != result_code) {
      UPLL_LOG_INFO("GetChildConfigKey failed");
      return result_code;
    }
    uint8_t rename_flag = 0;
    GET_USER_DATA_FLAGS(ckv_df, rename_flag);
    if (rename_flag & VTN_RENAME) {
      ConfigKeyVal *ckv_vtn = NULL;
      MoMgrImpl *vtn_mgr = reinterpret_cast<MoMgrImpl *>
          (const_cast<MoManager*>(GetMoManager(UNC_KT_VTN)));
      if (!vtn_mgr) {
        UPLL_LOG_INFO("Invalid Momgr");
        delete ckv_dupdf;
        return UPLL_RC_ERR_GENERIC;
      }
      result_code = vtn_mgr->GetChildConfigKey(ckv_vtn, ckv_df);
      if (UPLL_RC_SUCCESS != result_code) {
        UPLL_LOG_INFO("GetChildConfigKey failed");
        delete ckv_dupdf;
        return result_code;
      }
      result_code = vtn_mgr->GetRenamedControllerKey(ckv_vtn,
                                                     UPLL_DT_RUNNING,
                                                     dmi,
                                                     &ctrlr_dom);
      if (UPLL_RC_SUCCESS != result_code) {
        UPLL_LOG_INFO("GetRenamedControllerKey Failed %d",
                      result_code);

        delete ckv_vtn;
        delete ckv_dupdf;
        return result_code;
      }
      key_vtn_t *vtn_key = reinterpret_cast<key_vtn_t*>
          (ckv_vtn->get_key());
      key_vtn_dataflow *dup_dfkey =
          reinterpret_cast<key_vtn_dataflow_t *>(ckv_dupdf->get_key());
      uuu::upll_strncpy(dup_dfkey->vtn_key.vtn_name,
                        vtn_key->vtn_name, (kMaxLenVtnName+1));
      delete ckv_vtn;
    }
    IpcClientHandler  ipc_client;
    IpcRequest  ipc_req;
    memset(&ipc_req, 0, sizeof(ipc_req));
    memcpy(&(ipc_req.header), header, sizeof(IpcReqRespHeader));
    ipc_req.ckv_data = ckv_dupdf;
    IpcResponse *ipc_resp = &(ipc_client.ipc_resp);
    if (!ipc_client.SendReqToDriver(reinterpret_cast<const char *>
                                    (ctrlr_dom.ctrlr),
                                    (reinterpret_cast<char *>
                                     (ctrlr_dom.domain)),
                                    &ipc_req)) {
      UPLL_LOG_INFO("SendReqToDriver failed");
      delete ckv_dupdf;
      return ipc_resp->header.result_code;
    }
    if (ipc_resp->header.result_code != UPLL_RC_SUCCESS) {
      UPLL_LOG_INFO("Read from driver failed err code %d",
                    ipc_resp->header.result_code);
      delete ckv_dupdf;
      return ipc_resp->header.result_code;
    }
    uint32_t    arg = ipc_client.arg;
    UPLL_LOG_TRACE(" The Argument is %d", arg);
    pfc::core::ipc::ClientSession *cl_sess = ipc_client.cl_sess;
    uint32_t tot_flow_count = 0;
    int err = 0;
    if (0 != (err = cl_sess->getResponse(arg++, tot_flow_count))) {
      UPLL_LOG_TRACE("Failed to get total flow count field #%u."
                     " Err=%d", arg, err);
      if (is_first_ctrlr) {
        UPLL_LOG_TRACE("Inside if (is_head_node) and returning");
        delete ckv_dupdf;
        return UPLL_RC_ERR_GENERIC;
      } else {
        currentnode->addl_data->reason = UNC_DF_RES_SYSTEM_ERROR;
        UPLL_LOG_TRACE("Inside else and returning UNC_RC_SUCCESS");
        delete ckv_dupdf;
        return UPLL_RC_SUCCESS;
      }
    }
    UPLL_LOG_TRACE("Total flow count is %d", tot_flow_count);
    for (uint32_t i = 0; i < tot_flow_count; i++) {
      pfc_log_info("Reading flow %d from driver ", i);
      DataflowDetail *df_segm = new DataflowDetail(kidx_val_vtn_dataflow_cmn);
      df_segm->sessReadDataflow(*cl_sess, arg);
      pfc_flows.push_back(df_segm);
    }
    df_util->upll_pfc_flows.insert(std::pair<key_vtn_ctrlr_dataflow,
                                   vector<DataflowDetail *> >
                                   (vtn_ctrlr_df_key, pfc_flows));
    pfc_log_info("Got upll_pfc_flows from driver. flows.size=%" PFC_PFMT_SIZE_T
                 "", pfc_flows.size());
    delete ckv_dupdf;
  } else {
    pfc_flows = iter->second;
    pfc_log_info("Got pfc_flows from map. flows.size=%" PFC_PFMT_SIZE_T "",
                 pfc_flows.size());
  }
  for (uint32_t i = 0; i < pfc_flows.size(); i++) {
    DataflowDetail *df_segm = pfc_flows[i];
    bool is_vnode_match = false;
    DataflowCmn *df_cmn = new DataflowCmn(is_first_ctrlr, df_segm);
    if (!is_first_ctrlr) {
      bool match_result = df_cmn->check_match_condition
          (lastPfcNode->output_matches);
      if (!match_result) {
        UPLL_LOG_DEBUG("2nd flow (id=%" PFC_PFMT_u64
                       ") is not matching with 1st flow (id=%" PFC_PFMT_u64
                       ") so ignoring", df_cmn->df_segment->
                       vtn_df_common->flow_id,
                       currentnode->df_segment->vtn_df_common->flow_id);
        delete df_cmn;
        continue;
      }
    }
    result_code = MapVexternalToVbridge(ckv_df,
                                        df_cmn, &is_vnode_match, dmi);
    if (result_code == UPLL_RC_SUCCESS && !is_vnode_match) {
      UPLL_LOG_INFO("Ingress vnode does not match with filter vnode");
      delete df_cmn;
      continue;
    } else if (result_code == UPLL_RC_ERR_NO_SUCH_INSTANCE) {
      UPLL_LOG_INFO("Either ingress/Egress is a dynamic interface");
      UpdateReason(df_cmn, result_code);
    } else if (UPLL_RC_SUCCESS != result_code) {
      delete df_cmn;
      return result_code;
    }
    if (is_first_ctrlr) {
      df_cmn->apply_action();
      uint32_t ret = df_util->appendFlow(df_cmn);
      if (ret != 0) {
        delete df_cmn;
        UPLL_LOG_INFO("appendFlow failed");
        return UPLL_RC_ERR_GENERIC;
      }
    } else {
      UPLL_LOG_DEBUG("2nd flow (id=%" PFC_PFMT_u64
                     ") is matching with 1st flow (id=%" PFC_PFMT_u64  ")",
                     df_cmn->df_segment->vtn_df_common->flow_id,
                     currentnode->df_segment->vtn_df_common->flow_id);
      df_cmn->apply_action();
      currentnode->appendFlow(df_cmn, *(df_util->get_ctrlr_dom_count_map()));
      if (currentnode->addl_data->reason == UNC_DF_RES_EXCEEDS_HOP_LIMIT) {
        UPLL_LOG_DEBUG("flow reached max hop limit");
        delete df_cmn;
        continue;
      }
    }
  }
  vector<DataflowCmn* >* firstCtrlrFlows = df_util->get_firstCtrlrFlows();
  if (is_first_ctrlr) {
    if (firstCtrlrFlows->size() == 0) {
      return UPLL_RC_ERR_NO_SUCH_INSTANCE;
    }
  } else {
    if (currentnode->next.size() == 0 && currentnode->addl_data->reason ==
        UNC_DF_RES_SUCCESS) {  // Preserving old reason
      if (currentnode->df_segment->vtn_df_common->controller_type ==
          UNC_CT_PFC) {
        //  if parentnode is PFC type
        currentnode->addl_data->reason = UNC_DF_RES_FLOW_NOT_FOUND;
      } else {
        currentnode->addl_data->reason = UNC_DF_RES_DST_NOT_REACHED;
      }
      return UPLL_RC_SUCCESS;
    }
  }
  if (is_first_ctrlr) {
    vector<DataflowCmn *>::iterator iter_flow = firstCtrlrFlows->begin();
    while (iter_flow != firstCtrlrFlows->end()) {
      // Checking the particular flow is traversed
      DataflowCmn *traverse_flow_cmn =
          reinterpret_cast<DataflowCmn *>(*iter_flow);
      UPLL_LOG_TRACE("node:%s",
                     DataflowCmn::get_string(
                         *traverse_flow_cmn->
                         df_segment->vtn_df_common).c_str());
      if (traverse_flow_cmn->addl_data->reason !=
          UNC_DF_RES_EXCEEDS_FLOW_LIMIT) {
        result_code = CheckBoundaryAndTraverse(ckv_df, header,
                                               traverse_flow_cmn,
                                               traverse_flow_cmn,
                                               df_util,
                                               dmi);
        if (UPLL_RC_SUCCESS != result_code) {
          UPLL_LOG_TRACE("CheckBoundaryAndTraverse Failed %d\n",
                         result_code);
          return result_code;
        }
        vector<DataflowCmn *>::iterator match_flow = iter_flow + 1;
        unsigned int no_of_dataflow = 1;
        while (match_flow != firstCtrlrFlows->end()) {
          DataflowCmn *traverse_match_flow_cmn =
              reinterpret_cast<DataflowCmn *>(*match_flow);
          if ((traverse_flow_cmn->next.size() > 0) &&
              (traverse_match_flow_cmn->addl_data->reason !=
               UNC_DF_RES_EXCEEDS_FLOW_LIMIT)) {
            UPLL_LOG_DEBUG("Inside first ctrlr, if traversed == false");
            if (traverse_match_flow_cmn->
                CompareVtnDataflow(traverse_flow_cmn) == true)  {
              no_of_dataflow++;
              UPLL_LOG_DEBUG("CompareVtnDataflow returns true, no of df ="
                             "%d max_dataflow_traverse_count %d",
                             no_of_dataflow, upll_max_dataflow_traversal_);
              if (no_of_dataflow > upll_max_dataflow_traversal_) {
                UPLL_LOG_DEBUG("Setting flow limit to %p",
                               traverse_match_flow_cmn);
                traverse_match_flow_cmn->addl_data->reason =
                    UNC_DF_RES_EXCEEDS_FLOW_LIMIT;
                traverse_match_flow_cmn->addl_data->controller_count = 1;
              }
            }
          }
          match_flow++;
        }
      }
      iter_flow++;
      bypass_dom_set.clear();
    }
  } else {
    vector<DataflowCmn *>::iterator iter_flow = currentnode->next.begin();
    while (iter_flow != currentnode->next.end()) {
      // Checking the particular flow is traversed
      DataflowCmn *traverse_flow_cmn =
          reinterpret_cast<DataflowCmn *>(*iter_flow);
      UPLL_LOG_TRACE("node:%s",
                     DataflowCmn::get_string(*traverse_flow_cmn->
                                             df_segment->
                                             vtn_df_common).c_str());
      if (traverse_flow_cmn->addl_data->reason !=
          UNC_DF_RES_EXCEEDS_FLOW_LIMIT) {
        result_code = CheckBoundaryAndTraverse(ckv_df,
                                               header,
                                               *iter_flow,
                                               *iter_flow,
                                               df_util,
                                               dmi);
        UPLL_LOG_TRACE("CheckBoundaryAndTraverse nohead returned %d\n",
                       result_code);
        vector<DataflowCmn *>::iterator match_flow = iter_flow + 1;
        unsigned int no_of_dataflow = 1;
        while (match_flow != currentnode->next.end()) {
          DataflowCmn *traverse_match_flow_cmn =
              reinterpret_cast<DataflowCmn *>(*match_flow);
          if ((traverse_flow_cmn->next.size() > 0) &&
              (traverse_match_flow_cmn->addl_data->reason !=
               UNC_DF_RES_EXCEEDS_FLOW_LIMIT)) {
            UPLL_LOG_DEBUG("Inside if traversed = false if headnode\n");
            if (traverse_match_flow_cmn->CompareVtnDataflow(traverse_flow_cmn)
                == true) {
              no_of_dataflow++;
              UPLL_LOG_DEBUG("CompareVtndataflow returns true, node max_df "
                             "%d:%d\n",
                             no_of_dataflow,
                             upll_max_dataflow_traversal_);
              if (no_of_dataflow > upll_max_dataflow_traversal_) {
                traverse_match_flow_cmn->addl_data->reason =
                    UNC_DF_RES_EXCEEDS_FLOW_LIMIT;
              }
            }
          }
          match_flow++;
        }
      }
      iter_flow++;
    }
  }
  return UPLL_RC_SUCCESS;
}

upll_rc_t VtnDataflowMoMgr::UpdateReason(DataflowCmn *source_node,
                                         upll_rc_t result_code) {
  UPLL_FUNC_TRACE;
  if (result_code != UPLL_RC_SUCCESS) {
    switch (result_code) {
      case UPLL_RC_ERR_NO_SUCH_INSTANCE:
        if (source_node->df_segment->vtn_df_common->controller_type  ==
            UNC_CT_PFC)
          source_node->addl_data->reason = UNC_DF_RES_FLOW_NOT_FOUND;
        else
          source_node->addl_data->reason = UNC_DF_RES_DST_NOT_REACHED;
        break;
      case UPLL_RC_ERR_RESOURCE_DISCONNECTED:
      case UPLL_RC_ERR_CTR_DISCONNECTED:
        source_node->addl_data->reason = UNC_DF_RES_CTRLR_DISCONNECTED;
        break;
      case UPLL_RC_ERR_NOT_SUPPORTED_BY_CTRLR:
        source_node->addl_data->reason = UNC_DF_RES_OPERATION_NOT_SUPPORTED;
        break;
      default:
        source_node->addl_data->reason = UNC_DF_RES_SYSTEM_ERROR;
    }
  }
  return result_code;
}

upll_rc_t
VtnDataflowMoMgr::CheckBoundaryAndTraverse(ConfigKeyVal *ckv_df,
                                           IpcReqRespHeader *header,
                                           DataflowCmn *source_node,
                                           DataflowCmn *lastPfcNode,
                                           DataflowUtil *df_util,
                                           DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  ConfigKeyVal *ckv_remif = NULL;
  if_type vnif_type;
  if (!source_node || !lastPfcNode) {
    UPLL_LOG_DEBUG("DataflowCmn is Null");
    return UPLL_RC_ERR_GENERIC;
  }
  key_vtn_dataflow *vtn_df_key = reinterpret_cast<key_vtn_dataflow *>
      (ckv_df->get_key());
  ConfigKeyVal *ckv_ingress =
      reinterpret_cast<ConfigKeyVal *>(source_node->df_segment->ckv_egress);
  if (!ckv_ingress) {
    //    source_node->addl_data->reason = UNC_DF_RES_SUCCESS;
    UPLL_LOG_DEBUG("Egress interface not specified");
    return  UPLL_RC_SUCCESS;
  }
  VnodeChildMoMgr *vnif_mgr = reinterpret_cast<VnodeChildMoMgr *>(
      const_cast<MoManager *>(GetMoManager(ckv_ingress->get_key_type())));
  if (!vnif_mgr) {
    UPLL_LOG_ERROR("Invalid mgr\n");
    return UPLL_RC_ERR_GENERIC;
  }
  result_code = vnif_mgr->GetInterfaceType(ckv_ingress,
                                           UNC_VF_INVALID, vnif_type);
  if (result_code != UPLL_RC_SUCCESS) {
    return result_code;
  }
  if (vnif_type == kBoundaryInterface) {
    VlinkMoMgr *vlink_mgr = reinterpret_cast<VlinkMoMgr *>
        (const_cast<MoManager*>(GetMoManager(UNC_KT_VLINK)));
    if (!vlink_mgr) {
      UPLL_LOG_ERROR("Invalid mgr\n");
      return UPLL_RC_ERR_GENERIC;
    }
    result_code = vlink_mgr->GetRemoteIf(ckv_ingress, ckv_remif, dmi);
    if (result_code != UPLL_RC_SUCCESS) {
      UpdateReason(source_node, UPLL_RC_ERR_GENERIC);
      DELETE_IF_NOT_NULL(ckv_remif);
      return  UPLL_RC_SUCCESS;
    }
  } else {
    source_node->addl_data->reason = UNC_DF_RES_SUCCESS;
    UPLL_LOG_DEBUG("Egress interface is not boundary mapped");
    return  UPLL_RC_SUCCESS;
  }

  uint8_t *ctrlr_id = NULL;
  unc_keytype_ctrtype_t ctrlr_type = UNC_CT_UNKNOWN;
  GET_USER_DATA_CTRLR(ckv_remif, ctrlr_id);
  if (UNC_KT_VUNK_IF != ckv_remif->get_key_type()) {
    result_code = ValidateControllerCapability((const char *)(ctrlr_id),
                                               false, &ctrlr_type);
    if (result_code != UPLL_RC_SUCCESS) {
      UpdateReason(source_node, result_code);
      delete ckv_remif;
      return UPLL_RC_SUCCESS;
    }
  }
  if (ctrlr_type == UNC_CT_PFC) {
    uuu::upll_strncpy(vtn_df_key->vnode_id,
                      reinterpret_cast<key_vbr_if_t *>(ckv_remif->get_key())->
                      vbr_key.vbridge_name, (kMaxLenVnodeName + 1));
    SET_USER_DATA(ckv_df, ckv_remif);
    delete ckv_remif;
    result_code = TraversePFCController(ckv_df, header, source_node,
                                        lastPfcNode, df_util, dmi);
    if (result_code != UPLL_RC_SUCCESS) {
      UpdateReason(source_node, result_code);
      return UPLL_RC_SUCCESS;
    }
  } else {
    // Get the egress interfaces of the boundary(ies)
    // leading out of the vnp/vbypass domain
    // into the next neighboring controller domain.
    // (Karthi)  The Lisf of vnp/vbypass boundary information
    // available in the ckv_egress.
    // the Ingress for the PFC to VNP/Vbypass availbe in
    // ckv_remif first iteration.
    ConfigKeyVal *ckv_egress = NULL;
    bool found_inif = false;
    if_type vnif_type = kUnboundInterface;
    result_code = PopulateVnpOrVbypassBoundaryInfo(ckv_remif,
                                                   ckv_egress, dmi);
    if (result_code != UPLL_RC_SUCCESS) {
      UPLL_LOG_ERROR("Retrieval of boundary info failed\n %d\n", result_code);
      UpdateReason(source_node, UPLL_RC_ERR_GENERIC);
      return UPLL_RC_SUCCESS;
    }
    ConfigKeyVal *nxt_ckv = NULL;
    uint8_t *bypass_domain[2] = {NULL, NULL};
    std::pair<std::set<std::string>::iterator, bool> ret;
    GET_USER_DATA_DOMAIN(ckv_remif, bypass_domain[0]);
    ConfigKeyVal *ckv_tmp_nxt_ckv = NULL;
    nxt_ckv = ckv_egress;
    while (nxt_ckv) {
      ckv_tmp_nxt_ckv =  nxt_ckv->get_next_cfg_key_val();
      nxt_ckv->set_next_cfg_key_val(NULL);
      if (!found_inif && !strncmp(reinterpret_cast<char *>
                                  (reinterpret_cast<key_vnode_if_t *>
                                   (ckv_remif->get_key())->vnode_if_name),
                                  reinterpret_cast<char *>
                                  (reinterpret_cast<key_vnode_if_t *>
                                   (nxt_ckv->get_key())->vnode_if_name),
                                  kMaxLenInterfaceName + 1)) {
        found_inif = true;
        delete nxt_ckv;
        nxt_ckv = ckv_tmp_nxt_ckv;
        continue;
      }
      GET_USER_DATA_DOMAIN(nxt_ckv, bypass_domain[1]);
      if (ctrlr_type == UNC_CT_UNKNOWN) {
        ret = bypass_dom_set.insert(
            (string(reinterpret_cast<char *>(bypass_domain[1]))));
        UPLL_LOG_DEBUG("bypass egress domain %s", bypass_domain[1]);
        if (ret.second == false) {
          UPLL_LOG_INFO("bypass egress domain in loop %s", bypass_domain[1]);
          source_node->addl_data->reason = UNC_DF_RES_EXCEEDS_HOP_LIMIT;
          delete nxt_ckv;
          delete ckv_tmp_nxt_ckv;
          break;
        }
      }
      DataflowDetail *df_segment =
          new DataflowDetail(kidx_val_vtn_dataflow_cmn, ctrlr_type);
      DataflowCmn *df_cmn = new DataflowCmn(false, df_segment);
      uuu::upll_strncpy(
          df_cmn->df_segment->vtn_df_common->ingress_domain,
          bypass_domain[0], (kMaxLenDomainId + 1));
      uuu::upll_strncpy(
          df_cmn->df_segment->vtn_df_common->egress_domain,
          bypass_domain[1], (kMaxLenDomainId + 1));
      uuu::upll_strncpy(
          df_cmn->df_segment->vtn_df_common->controller_id,
          ctrlr_id, (kMaxLenCtrlrId + 1));
      df_cmn->df_segment->vtn_df_common->controller_type = ctrlr_type;
      uuu::upll_strncpy(df_cmn->df_segment->vtn_df_common->ingress_vnode,
                        reinterpret_cast<key_vnode_if_t *>
                        (ckv_remif->get_key())->vnode_key.vnode_name,
                        (kMaxLenVnodeName + 1));
      uuu::upll_strncpy(df_cmn->df_segment->vtn_df_common->
                        ingress_vinterface, reinterpret_cast<key_vnode_if_t *>
                        (ckv_remif->get_key())->vnode_if_name,
                        (kMaxLenInterfaceName + 1));
      uuu::upll_strncpy(df_cmn->df_segment->vtn_df_common->egress_vnode,
                        reinterpret_cast<key_vnode_if_t *>
                        (nxt_ckv->get_key())->vnode_key.vnode_name,
                        (kMaxLenVnodeName + 1));
      uuu::upll_strncpy(df_cmn->df_segment->vtn_df_common->egress_vinterface,
                        reinterpret_cast<key_vnode_if_t *>
                        (nxt_ckv->get_key())->vnode_if_name,
                        (kMaxLenInterfaceName + 1));
      source_node->appendFlow(df_cmn, *(df_util->get_ctrlr_dom_count_map()));
      result_code =  vnif_mgr->GetInterfaceType(nxt_ckv,
                                                UNC_VF_INVALID, vnif_type);
      if (vnif_type != kBoundaryInterface) {
        delete nxt_ckv;
        nxt_ckv = ckv_tmp_nxt_ckv;
        continue;
      }
      df_cmn->df_segment->ckv_egress = nxt_ckv;
      SET_USER_DATA(ckv_df, nxt_ckv);
      // Traverse the VNP/Vbypass boundary nodes.
      result_code = CheckBoundaryAndTraverse(ckv_df,
                                             header,
                                             df_cmn,
                                             lastPfcNode,
                                             df_util,
                                             dmi);
      if (result_code != UPLL_RC_SUCCESS) {
        UPLL_LOG_ERROR("Retrieval of boundary info failed\n %d\n",
                       result_code);
        UpdateReason(source_node, result_code);
        DELETE_IF_NOT_NULL(ckv_remif);
        DELETE_IF_NOT_NULL(ckv_tmp_nxt_ckv);
        return UPLL_RC_SUCCESS;
      }
      nxt_ckv = ckv_tmp_nxt_ckv;
    }
    delete ckv_remif;
  }
  return result_code;
}

upll_rc_t VtnDataflowMoMgr::GetChildConfigKey(ConfigKeyVal *&okey,
                                              ConfigKeyVal *parent_key) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  key_vtn_dataflow *vtn_dfkey = NULL;
  if (okey && okey->get_key()) {
    vtn_dfkey = reinterpret_cast<key_vtn_dataflow *>(
        okey->get_key());
  } else {
    vtn_dfkey = reinterpret_cast<key_vtn_dataflow *>(
        ConfigKeyVal::Malloc(sizeof(key_vtn_dataflow)));
  }
  void *pkey;
  if (parent_key == NULL) {
    if (!okey)
      okey = new ConfigKeyVal(UNC_KT_VTN_DATAFLOW,
                              IpctSt::kIpcStKeyVtnDataflow, vtn_dfkey, NULL);
    else if (okey->get_key() != vtn_dfkey)
      okey->SetKey(IpctSt::kIpcStKeyVtnDataflow, vtn_dfkey);
    return UPLL_RC_SUCCESS;
  } else {
    pkey = parent_key->get_key();
  }
  if (!pkey) {
    if (!okey || !(okey->get_key()))
      ConfigKeyVal::Free(vtn_dfkey);
    return UPLL_RC_ERR_GENERIC;
  }
  switch (parent_key->get_key_type()) {
    case UNC_KT_ROOT:
      break;
    case UNC_KT_VTN_DATAFLOW:
      memcpy(vtn_dfkey, reinterpret_cast<key_vtn_dataflow *>(pkey),
             sizeof(key_vtn_dataflow));
      break;
    case UNC_KT_VTN:
      uuu::upll_strncpy(vtn_dfkey->vtn_key.vtn_name,
                        reinterpret_cast<key_vtn *>(pkey)->vtn_name,
                        (kMaxLenVtnName+1));
      break;
    default:
      if (!okey || !(okey->get_key())) {
        ConfigKeyVal::Free(vtn_dfkey);
      }
      return UPLL_RC_ERR_GENERIC;
  }
  if (!okey)
    okey = new ConfigKeyVal(UNC_KT_VTN_DATAFLOW,
                            IpctSt::kIpcStKeyVtnDataflow, vtn_dfkey, NULL);
  else if (okey->get_key() != vtn_dfkey)
    okey->SetKey(IpctSt::kIpcStKeyVtnDataflow, vtn_dfkey);
  SET_USER_DATA(okey, parent_key);
  return result_code;
}

}  // namespace kt_momgr
}  // namespace upll
}  // namespace unc
