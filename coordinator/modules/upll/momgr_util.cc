/*
 * Copyright (c) 2012-2014 NEC Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */
//  #include <iostream>
#include <map>
#include <string>
#include <list>
#include "momgr_impl.hh"
#include "vtn_momgr.hh"
#include "vbr_momgr.hh"
#include "uncxx/upll_log.hh"
#include "vlink_momgr.hh"
#include "vnode_momgr.hh"
#include "vnode_child_momgr.hh"

#define IMPORT_READ_FAILURE 0xFF
namespace unc {
namespace upll {
namespace kt_momgr {

upll_rc_t MoMgrImpl::GetInstanceCount(ConfigKeyVal *ikey,
                                      char *ctrlr_id,
                                      upll_keytype_datatype_t dt_type,
                                      uint32_t *count,
                                      DalDmlIntf *dmi,
                                      MoMgrTables tbl) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code;
  const uudst::kDalTableIndex tbl_index = GetTable(tbl, dt_type);
  if (tbl_index >= uudst::kDalNumTables) {
    UPLL_LOG_DEBUG(" Invalid Table index - %d", tbl_index);
    return UPLL_RC_ERR_GENERIC;
  }
  if (ikey == NULL) {
    UPLL_LOG_DEBUG("Invalid Param ikey/ctrlr_id");
    return UPLL_RC_ERR_GENERIC;
  }
  DbSubOp dbop = { kOpReadCount, kOpMatchCtrlr, kOpInOutNone };
  if (!ctrlr_id || strlen(ctrlr_id) == 0) {
    dbop.matchop = kOpMatchNone;
  } else {
    SET_USER_DATA_CTRLR(ikey, ctrlr_id);
  }

  DalBindInfo *dal_bind_info = new DalBindInfo(tbl_index);
  result_code = BindAttr(dal_bind_info, ikey, UNC_OP_READ, dt_type, dbop, tbl);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_DEBUG("BindAttr returns error %d", result_code);
    delete dal_bind_info;
    return UPLL_RC_ERR_GENERIC;
  }
  result_code = DalToUpllResCode(
      dmi->GetRecordCount((
              dt_type == UPLL_DT_STATE)?UPLL_DT_RUNNING:dt_type,
          tbl_index,
          dal_bind_info,
          count));
  delete dal_bind_info;
  return result_code;
}

upll_rc_t MoMgrImpl::IsRenamed(ConfigKeyVal *ikey,
                               upll_keytype_datatype_t dt_type,
                               DalDmlIntf *dmi,
                               uint8_t &rename) {
  UPLL_FUNC_TRACE;
  DbSubOp dbop = { kOpReadSingle, kOpMatchNone, kOpInOutFlag | kOpInOutCtrlr
    | kOpInOutDomain };
  ConfigKeyVal *okey = NULL;
  upll_rc_t result_code;
  MoMgrTables tbl = MAINTBL;
  /* rename is set and dt_type is running/audit implies
   * operaton is delete and ikey has to be populated with
   * val from db.
   */
  if (rename &&
      ((dt_type == UPLL_DT_RUNNING) ||
       (dt_type == UPLL_DT_AUDIT))) {
    okey = ikey;
  } else  {
    result_code = GetChildConfigKey(okey, ikey);
    if (result_code != UPLL_RC_SUCCESS) {
      UPLL_LOG_TRACE("Returning error %d", result_code);
      return result_code;
    }
  }

  if (UNC_KT_VTN == table[MAINTBL]->get_key_type()) {
    controller_domain_t ctrlr_dom;
    ctrlr_dom.ctrlr = NULL;
    ctrlr_dom.domain = NULL;
    GET_USER_DATA_CTRLR_DOMAIN(okey, ctrlr_dom);
    /* if controller and domain is present then bind it for
     * match and get the exact information from vtn controller
     * table
     */
    if (ctrlr_dom.ctrlr != NULL && ctrlr_dom.domain != NULL) {
      dbop.matchop = kOpMatchCtrlr | kOpMatchDomain;
      dbop.inoutop = kOpInOutFlag;
    }
    tbl = CTRLRTBL;
  }
  result_code = ReadConfigDB(okey, dt_type, UNC_OP_READ, dbop, dmi,
                             tbl);
  if ((result_code != UPLL_RC_SUCCESS) &&
      (result_code != UPLL_RC_ERR_NO_SUCH_INSTANCE))  {
    UPLL_LOG_DEBUG("Returning error code %d", result_code);
    if (okey != ikey) delete okey;
    return UPLL_RC_ERR_GENERIC;
  }
  if (okey != ikey)
    SET_USER_DATA(ikey, okey);
  GET_USER_DATA_FLAGS(okey, rename);
#if 0
  rename &= RENAME;
#else
  GET_RENAME_FLAG(rename, ikey->get_key_type())
#endif
      if (okey != ikey) delete okey;
  return UPLL_RC_SUCCESS;
}

upll_rc_t MoMgrImpl::RenameChildren(ConfigKeyVal *ikey,
                                    upll_keytype_datatype_t dt_type,
                                    DalDmlIntf *dmi,
                                    MoMgrTables tbl) {
  UPLL_FUNC_TRACE;
  ConfigKeyVal *tkey = NULL;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  bool fail = false;

  char rename = 0;
  GET_USER_DATA_FLAGS(ikey, rename);
  rename &= RENAME;
  for (int i = 0; i < nchild; i++) {
    unc_key_type_t ktype = child[i];
    MoMgrImpl *mgr = reinterpret_cast<MoMgrImpl *>
        (const_cast<MoManager *>(GetMoManager(ktype)));
    if (!mgr) {
      UPLL_LOG_DEBUG("Invalid mgr");
      return UPLL_RC_ERR_GENERIC;
    }
    // cout << *ikey << ktype << " " << mgr << "\n";
    result_code = mgr->GetChildConfigKey(tkey, ikey);
    if (UPLL_RC_SUCCESS != result_code) {
      UPLL_LOG_DEBUG("GetChildConfigKey failed %d", result_code);
      return result_code;
    }
    // cout << "Renaming keytype " << ktype << " " << *tkey << "\n";
    DbSubOp dbop = { kOpReadMultiple, kOpMatchNone, kOpInOutFlag };
    result_code = mgr->ReadConfigDB(tkey,
                                    dt_type,
                                    UNC_OP_READ,
                                    dbop,
                                    dmi,
                                    tbl);
    ConfigKeyVal *tmp = tkey;
    while (tmp != NULL) {
      uint8_t child_rename = 0;
      GET_USER_DATA_FLAGS(tmp, child_rename);
      child_rename &= RENAME;
      if (child_rename == rename) continue;
      rename &= RENAME;
      SET_USER_DATA_FLAGS(tmp, rename);
      result_code = mgr->UpdateConfigDB(tkey,
                                        dt_type,
                                        UNC_OP_UPDATE,
                                        dmi,
                                        tbl);
      if (result_code != UPLL_RC_SUCCESS) {
        UPLL_LOG_DEBUG("UpdateConfigDB failed with error code %d",
                       result_code);
        DELETE_IF_NOT_NULL(tkey);
        return result_code;
      }
      tmp = tmp->get_next_cfg_key_val();
    }
    delete tkey;
    tkey = NULL;
    result_code = mgr->RenameChildren(ikey, dt_type, dmi);
    if (result_code != UPLL_RC_SUCCESS) {
      UPLL_LOG_DEBUG("Renamed failed with error code %d", result_code);
      fail = true;
    }
  }
  return ((fail == true) ? UPLL_RC_ERR_GENERIC : UPLL_RC_SUCCESS);
}

upll_rc_t MoMgrImpl::DeleteChildren(ConfigKeyVal *ikey,
                                    ConfigKeyVal *pkey,
                                    upll_keytype_datatype_t dt_type,
                                    DalDmlIntf *dmi,
                                    MoMgrTables tbl) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  bool fail = false;
  unc_key_type_t ktype;

  for (int i = nchild; i > 0; i--) {
    ConfigKeyVal *tkey = NULL;
    ktype = child[(i - 1)];
    MoMgrImpl *mgr = reinterpret_cast<MoMgrImpl *>
        (const_cast<MoManager*>(GetMoManager(ktype)));
    if (!mgr) {
      UPLL_LOG_DEBUG("Invalid mgr %d", ktype);
      continue;
    }
    result_code = mgr->GetChildConfigKey(tkey, pkey);
    if (UPLL_RC_SUCCESS != result_code) {
      UPLL_LOG_DEBUG("GetChildConfigKey failed %d", result_code);
      return result_code;
    }
    /* For deleting the vnode rename table for the vtn or vnode
     * need no to match the controller and domain
     */
    memset(tkey->get_user_data(), 0 , sizeof(key_user_data_t));
    result_code = mgr->DeleteChildren(tkey, pkey, dt_type, dmi);
    DELETE_IF_NOT_NULL(tkey);
  }
  bool kt_flag = false;
  IS_POM_KT(GetMoMgrKeyType(MAINTBL, dt_type), kt_flag);
  if (kt_flag) {
    result_code = DeleteChildrenPOM(ikey, dt_type, dmi);
    if (UPLL_RC_SUCCESS != result_code) {
      UPLL_LOG_DEBUG("DeleteChildrenPOM failed %d", result_code);
      return result_code;
    }
  } else {
    /* Delete all the tables for this momgr
     * RENAMETBL to be deleted only once */
    for (int j = get_ntable(); j > MAINTBL; j--) {
      if ((GetTable((MoMgrTables)(j - 1), dt_type) >= uudst::kDalNumTables)) {
        continue;
      }
      /* Match Controller and domain is not need for delete children*/
      DbSubOp dbop = {kOpNotRead, kOpMatchNone, kOpInOutNone};
      result_code = UpdateConfigDB(ikey, dt_type, UNC_OP_DELETE, dmi, &dbop,
                                   (MoMgrTables)(j - 1));
      result_code = (result_code == UPLL_RC_ERR_NO_SUCH_INSTANCE)?
          UPLL_RC_SUCCESS : result_code;
      if (result_code != UPLL_RC_SUCCESS) {
        UPLL_LOG_DEBUG("DeleteChild failed with result_code %d", result_code);
        fail = true;
      }
    }
  }
  return ((fail == true) ? UPLL_RC_ERR_GENERIC : UPLL_RC_SUCCESS);
}

upll_rc_t MoMgrImpl::RestoreChildren(ConfigKeyVal *&ikey,
                                     upll_keytype_datatype_t dest_cfg,
                                     upll_keytype_datatype_t src_cfg,
                                     DalDmlIntf *dmi,
                                     IpcReqRespHeader *req,
                                     MoMgrTables tbl) {
  UPLL_FUNC_TRACE;
  ConfigKeyVal *tkey = NULL;
  ConfigKeyVal *start_ptr = NULL;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  bool restore_flag = true;
  int index = MAINTBL;
  int i = 0;
  MoMgrImpl *chld_mgr = NULL;
  while (true) {
    unc_key_type_t ktype = (unc_key_type_t)0;
    unc_key_type_t instance_key_type = GetMoMgrKeyType(tbl, src_cfg);

    result_code  = GetChildConfigKey(tkey, ikey);
    if (UPLL_RC_SUCCESS != result_code) {
      return result_code;
    }

    DbSubOp dbop = { kOpReadMultiple, kOpMatchNone,
      kOpInOutFlag | kOpInOutCtrlr | kOpInOutDomain };
    result_code = ReadConfigDB(tkey,
                               src_cfg,
                               UNC_OP_READ,
                               dbop,
                               dmi,
                               (MoMgrTables)index);
    if (UPLL_RC_SUCCESS != result_code
        && UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code) {
      UPLL_LOG_DEBUG("ReadConfigDB Failed");
      DELETE_IF_NOT_NULL(tkey);
      return result_code;
    }

    if (UPLL_RC_ERR_NO_SUCH_INSTANCE == result_code) {
      DELETE_IF_NOT_NULL(tkey);
      return result_code;
    }

    start_ptr = tkey;
    while (tkey != NULL) {
      result_code = CreateCandidateMo(req, tkey, dmi, restore_flag);
      if (result_code != UPLL_RC_SUCCESS) {
        UPLL_LOG_DEBUG("CreateCandidateMo failed with error code %d\n",
                       result_code);
        DELETE_IF_NOT_NULL(start_ptr);
        return result_code;
      }

      result_code = CheckExistenceInRenameTable(ikey,
                                                src_cfg,
                                                instance_key_type,
                                                dmi);
      if (result_code != UPLL_RC_SUCCESS) {
        UPLL_LOG_DEBUG("CheckExistenceInRenameTable failed with err code %d\n",
                       result_code);
        DELETE_IF_NOT_NULL(start_ptr);
        return result_code;
      }
      tkey = tkey->get_next_cfg_key_val();
    }
    DELETE_IF_NOT_NULL(start_ptr);

    if (nchild == 0) {
      return UPLL_RC_SUCCESS;
    }

    while (true) {
      ktype = child[i];
      chld_mgr = reinterpret_cast<MoMgrImpl *>
          (const_cast<MoManager *>(GetMoManager(ktype)));
      if (!chld_mgr) {
        UPLL_LOG_DEBUG("Invalid mgr");
        return UPLL_RC_ERR_GENERIC;
      }

      result_code = chld_mgr->RestoreChildren(ikey,
                                              dest_cfg,
                                              src_cfg,
                                              dmi,
                                              req);

      if ((UPLL_RC_SUCCESS != result_code) &&
          (UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code)) {
        UPLL_LOG_DEBUG("Restored failed with error code %d\n", result_code);
        return UPLL_RC_ERR_GENERIC;
      }

      if ((nchild-1) <= i)
        return UPLL_RC_SUCCESS;
      i++;
    }
  }
}

upll_rc_t MoMgrImpl::CheckExistenceInRenameTable(
    ConfigKeyVal *&req,
    upll_keytype_datatype_t dt_type,
    unc_key_type_t instance_key_type,
    DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  ConfigKeyVal *tkey = NULL;
  ConfigKeyVal *ctrl_key = NULL;
  MoMgrImpl *vtn_mgr = NULL;
  if ((instance_key_type == UNC_KT_VBRIDGE) ||
      (instance_key_type == UNC_KT_VROUTER)) {
    result_code  = GetChildConfigKey(tkey, req);
    if (UPLL_RC_SUCCESS != result_code) {
      return result_code;
    }
    result_code = GetParentConfigKey(ctrl_key, tkey);
    if (result_code != UPLL_RC_SUCCESS) {
      DELETE_IF_NOT_NULL(tkey);
      UPLL_LOG_DEBUG("GetParentConfigKey Failed %d", result_code);
      return result_code;
    }
    DbSubOp dbop = { kOpReadSingle, kOpMatchCtrlr|kOpMatchDomain,
      kOpInOutCtrlr | kOpInOutDomain };
    vtn_mgr = reinterpret_cast<MoMgrImpl *>
        (const_cast<MoManager *>(GetMoManager(UNC_KT_VTN)));
    if (!vtn_mgr) {
      UPLL_LOG_DEBUG("Instance is NULL");
      DELETE_IF_NOT_NULL(ctrl_key);
      DELETE_IF_NOT_NULL(tkey);
      return UPLL_RC_ERR_GENERIC;
    }
    result_code = vtn_mgr->ReadConfigDB(ctrl_key,
                                        UPLL_DT_CANDIDATE,
                                        UNC_OP_READ,
                                        dbop,
                                        dmi ,
                                        (MoMgrTables)RENAMETBL);
    if ((UPLL_RC_SUCCESS != result_code) &&
        (UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code)) {
      UPLL_LOG_DEBUG("ReadConfigDB Failed in candidateDB %d", result_code);
      DELETE_IF_NOT_NULL(ctrl_key);
      DELETE_IF_NOT_NULL(tkey);
      return result_code;
    }
    if ((UPLL_RC_ERR_NO_SUCH_INSTANCE == result_code)) {
      result_code = vtn_mgr->ReadConfigDB(ctrl_key, dt_type, UNC_OP_READ,
                                          dbop, dmi , (MoMgrTables)RENAMETBL);
      if ((UPLL_RC_SUCCESS != result_code) &&
          (UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code)) {
        UPLL_LOG_DEBUG("ReadConfigDB Failed in RunningDB %d", result_code);
        DELETE_IF_NOT_NULL(ctrl_key);
        DELETE_IF_NOT_NULL(tkey);
        return result_code;
      }
      if (UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code) {
        val_rename_vtn *ival = reinterpret_cast<val_rename_vtn *>
            (GetVal(ctrl_key));
        if (ival == NULL) {
          UPLL_LOG_DEBUG("Null Val structure");
          DELETE_IF_NOT_NULL(ctrl_key);
          DELETE_IF_NOT_NULL(tkey);
          return UPLL_RC_ERR_GENERIC;
        }
        ival->valid[0] = UNC_VF_VALID;
        dbop.readop = kOpNotRead;
        result_code = vtn_mgr->UpdateConfigDB(ctrl_key,
                                              UPLL_DT_CANDIDATE,
                                              UNC_OP_CREATE,
                                              dmi,
                                              &dbop,
                                              (MoMgrTables)RENAMETBL);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_INFO("UpdateConfigDB failed with error code %d\n",
                        result_code);
          DELETE_IF_NOT_NULL(ctrl_key);
          DELETE_IF_NOT_NULL(tkey);
          return result_code;
        }
      }
    }
    DELETE_IF_NOT_NULL(ctrl_key);
    DbSubOp dboper = { kOpReadSingle, kOpMatchCtrlr|kOpMatchDomain,
      kOpInOutCtrlr | kOpInOutDomain };
    result_code = ReadConfigDB(tkey,
                               dt_type,
                               UNC_OP_READ,
                               dboper,
                               dmi,
                               (MoMgrTables)RENAMETBL);
    if ((UPLL_RC_SUCCESS != result_code) &&
        (UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code)) {
      UPLL_LOG_DEBUG("ReadConfigDB Failed");
      DELETE_IF_NOT_NULL(ctrl_key);
      DELETE_IF_NOT_NULL(tkey);
      return result_code;
    }
    if (UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code) {
      val_rename_vnode *ival = reinterpret_cast<val_rename_vnode *>
          (GetVal(tkey));
      if (ival == NULL) {
        UPLL_LOG_DEBUG("Null Val structure");
        DELETE_IF_NOT_NULL(ctrl_key);
        DELETE_IF_NOT_NULL(tkey);
        return UPLL_RC_ERR_GENERIC;
      }
      ival->valid[UPLL_CTRLR_VTN_NAME_VALID] = UNC_VF_VALID;
      ival->valid[UPLL_CTRLR_VNODE_NAME_VALID] = UNC_VF_VALID;
      dboper.readop = kOpNotRead;
      result_code = UpdateConfigDB(tkey,
                                   UPLL_DT_CANDIDATE,
                                   UNC_OP_CREATE,
                                   dmi,
                                   &dboper,
                                   (MoMgrTables)RENAMETBL);
      if (result_code != UPLL_RC_SUCCESS) {
        UPLL_LOG_DEBUG("UpdateConfigDB failed with error code %d\n",
                       result_code);
        DELETE_IF_NOT_NULL(tkey);
        DELETE_IF_NOT_NULL(ctrl_key);
        return result_code;
      }
    }
  }

  if ((instance_key_type == UNC_KT_FLOWLIST) ||
      (instance_key_type == UNC_KT_POLICING_PROFILE)) {
    result_code  = GetChildConfigKey(tkey, req);
    if (UPLL_RC_SUCCESS != result_code) {
      return result_code;
    }
    DbSubOp dboper = { kOpReadSingle, kOpMatchCtrlr|kOpMatchDomain,
      kOpInOutCtrlr | kOpInOutDomain };
    result_code = ReadConfigDB(tkey,
                               dt_type,
                               UNC_OP_READ,
                               dboper,
                               dmi,
                               (MoMgrTables)RENAMETBL);
    if ((UPLL_RC_SUCCESS != result_code) &&
        (UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code)) {
      UPLL_LOG_DEBUG("ReadConfigDB Failed");
      DELETE_IF_NOT_NULL(tkey);
      return result_code;
    }
    if (UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code) {
      val_rename_flowlist *ival = reinterpret_cast<val_rename_flowlist *>
          (GetVal(tkey));
      if (ival == NULL) {
        UPLL_LOG_DEBUG("Null Val structure");
        DELETE_IF_NOT_NULL(tkey);
        return UPLL_RC_ERR_GENERIC;
      }
      ival->valid[0] = UNC_VF_VALID;
      dboper.readop = kOpNotRead;
      result_code = UpdateConfigDB(tkey,
                                   UPLL_DT_CANDIDATE,
                                   UNC_OP_CREATE,
                                   dmi,
                                   &dboper,
                                   (MoMgrTables)RENAMETBL);
      if (result_code != UPLL_RC_SUCCESS) {
        UPLL_LOG_DEBUG("UpdateConfigDB failed with error code %d\n",
                       result_code);
        DELETE_IF_NOT_NULL(tkey);
        return result_code;
      }
    }
  }
  DELETE_IF_NOT_NULL(tkey);
  return UPLL_RC_SUCCESS;
}

upll_rc_t MoMgrImpl::DiffConfigDB(upll_keytype_datatype_t dt_cfg1,
                                  upll_keytype_datatype_t dt_cfg2,
                                  unc_keytype_operation_t op,
                                  ConfigKeyVal *&req,
                                  ConfigKeyVal *&nreq,
                                  DalCursor **cfg1_cursor,
                                  DalDmlIntf *dmi,
                                  MoMgrTables tbl) {
  UPLL_FUNC_TRACE;

  upll_rc_t result_code;
  result_code = DiffConfigDB(dt_cfg1, dt_cfg2, op, req, nreq, cfg1_cursor, dmi,
                             NULL, tbl);
  return result_code;
}

upll_rc_t MoMgrImpl::DiffConfigDB(upll_keytype_datatype_t dt_cfg1,
                                  upll_keytype_datatype_t dt_cfg2,
                                  unc_keytype_operation_t op,
                                  ConfigKeyVal *&req,
                                  ConfigKeyVal *&nreq,
                                  DalCursor **cfg1_cursor,
                                  DalDmlIntf *dmi,
                                  uint8_t *ctrlr_id,
                                  MoMgrTables tbl, bool read_withcs) {
  UPLL_FUNC_TRACE;
  const uudst::kDalTableIndex tbl_index = GetTable(tbl, dt_cfg1);
  if (tbl_index >= uudst::kDalNumTables) {
    UPLL_LOG_DEBUG(" Invalid Table index - %d", tbl_index);
    return UPLL_RC_ERR_GENERIC;
  }
  UPLL_LOG_DEBUG("Table Index %d Table %d Operation op %d",
                 tbl_index, tbl, op);
  upll_rc_t result_code;
  DalResultCode db_result = uud::kDalRcSuccess;
  result_code = GetChildConfigKey(req, NULL);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_ERROR("Error from GetGetChildConfigKey for table(%d)", tbl_index);
    return result_code;
  }
  DbSubOp dbop = { kOpReadDiff, kOpMatchCtrlr | kOpMatchDomain,
    kOpInOutFlag | kOpInOutCtrlr | kOpInOutDomain };
  if (UNC_OP_DELETE == op)
    dbop.matchop = kOpMatchCtrlr | kOpMatchDomain;
  uint16_t max_record_count = 0;
#if 0
  if (ctrlr_id) {
    dbop.inoutop &= ~kOpInOutCtrlr;
    SET_USER_DATA_CTRLR(req, ctrlr_id)
  }
#endif
  if (tbl == CTRLRTBL) {
    dbop.inoutop |= kOpInOutCs;
  }
  if (op == UNC_OP_UPDATE) {
    if (tbl == CTRLRTBL)
      dbop.matchop |= kOpMatchFlag;
    else
      dbop.matchop = kOpMatchFlag;
    dbop.readop |= kOpReadDiffUpd;
    if (read_withcs)
      dbop.inoutop |= kOpInOutCs;
  }
  if (dt_cfg2 == UPLL_DT_AUDIT) {
    dbop.matchop |= kOpMatchCs;
    dbop.matchop &= ~kOpMatchFlag;
  }
  DalBindInfo *binfo_cfg1 = new DalBindInfo(tbl_index);
  result_code = BindAttr(binfo_cfg1, req, UNC_OP_READ, dt_cfg1, dbop, tbl);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_DEBUG("Error from BindAttr for table(%d)", tbl_index);
    delete binfo_cfg1;
    DELETE_IF_NOT_NULL(req);
    req = NULL;
    return result_code;
  }

  switch (op) {
    case UNC_OP_DELETE:
      db_result = dmi->GetDeletedRecords(dt_cfg1, dt_cfg2, tbl_index,
                                         max_record_count, binfo_cfg1,
                                         cfg1_cursor);
      break;
    case UNC_OP_CREATE:
      db_result = dmi->GetCreatedRecords(dt_cfg1, dt_cfg2, tbl_index,
                                         max_record_count, binfo_cfg1,
                                         cfg1_cursor);
      break;
    case UNC_OP_UPDATE: {
      DalBindInfo *binfo_cfg2 = new DalBindInfo(tbl_index);
      result_code = GetChildConfigKey(nreq, NULL);
      if (result_code != UPLL_RC_SUCCESS) {
        UPLL_LOG_DEBUG("Failed in GetChildConfigKey");
        delete binfo_cfg1;
        delete binfo_cfg2;
        delete req;
        req = NULL;
        return result_code;
      }
      result_code = BindAttr(
          binfo_cfg2, nreq, UNC_OP_READ,
          ((dt_cfg2 == UPLL_DT_RUNNING)?UPLL_DT_STATE:dt_cfg2), dbop, tbl);
      if (result_code != UPLL_RC_SUCCESS) {
        UPLL_LOG_DEBUG("Error from BindAttr for table(%d)", tbl_index);
        delete binfo_cfg1;
        delete binfo_cfg2;
        delete req;
        delete nreq;
        req = nreq = NULL;
        return result_code;
      }
      db_result = dmi->GetUpdatedRecords(dt_cfg1, dt_cfg2, tbl_index,
                                         max_record_count, binfo_cfg1,
                                         binfo_cfg2, cfg1_cursor);
      // For Success case, binfo_cfg2 will be deleted by the caller.
      if (db_result != uud::kDalRcSuccess) {
        delete binfo_cfg2;
      }
      break;
    }
    default:
      break;
  }
  result_code = DalToUpllResCode(db_result);
  // For Success case, binfo_cfg1 will be deleted by the caller.
  if (result_code != UPLL_RC_SUCCESS) {
    delete binfo_cfg1;
  }
  return result_code;
}

upll_rc_t MoMgrImpl::ReadConfigDB(ConfigKeyVal *ikey,
                                  upll_keytype_datatype_t dt_type,
                                  unc_keytype_operation_t op,
                                  DbSubOp dbop,
                                  DalDmlIntf *dmi,
                                  MoMgrTables tbl) {
  UPLL_FUNC_TRACE;
  uint32_t sibling_count = 0;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  result_code = ReadConfigDB(ikey, dt_type, op, dbop, sibling_count, dmi, tbl);
  return result_code;
}

upll_rc_t MoMgrImpl::ReadConfigDB(ConfigKeyVal *ikey,
                                  upll_keytype_datatype_t dt_type,
                                  unc_keytype_operation_t op,
                                  DbSubOp dbop,
                                  uint32_t &sibling_count,
                                  DalDmlIntf *dmi,
                                  MoMgrTables tbl) {
  UPLL_FUNC_TRACE;
  const uudst::kDalTableIndex tbl_index = GetTable(tbl, dt_type);
  if (tbl_index >= uudst::kDalNumTables) {
    UPLL_LOG_DEBUG(" Invalid Table index - %d", tbl_index);
    return UPLL_RC_ERR_GENERIC;
  }
  DalCursor *dal_cursor_handle = NULL;
  UPLL_LOG_TRACE("tbl_index is %d", tbl_index);
  if (!READ_OP(op)) {
    UPLL_LOG_INFO("Exiting MoMgrImpl::ReadConfigDB");
    return UPLL_RC_ERR_GENERIC;
  }
  DalBindInfo *dal_bind_info = new DalBindInfo(tbl_index);
  upll_rc_t result_code;
  DalResultCode db_result = uud::kDalRcGeneralError;
#if 0
  uint16_t max_record_count = 1;
#endif
  ConfigKeyVal *tkey = NULL;
  result_code = BindAttr(dal_bind_info, ikey, op, dt_type, dbop, tbl);
  if (result_code != UPLL_RC_SUCCESS) {
    if (dal_bind_info) delete dal_bind_info;
    UPLL_LOG_INFO("Exiting MoMgrImpl::ReadConfigDB result code %d",
                  result_code);
    return result_code;
  }
  dt_type = (dt_type == UPLL_DT_STATE) ? UPLL_DT_RUNNING : dt_type;
  switch (op) {
    case UNC_OP_READ_SIBLING_COUNT:
      {
        db_result = dmi->GetRecordCount(dt_type, tbl_index, dal_bind_info,
                                        &sibling_count);
        uint32_t *sib_count =
            reinterpret_cast<uint32_t*>(ConfigKeyVal::Malloc
                                        (sizeof(uint32_t)));
        *sib_count = sibling_count;
        ikey->SetCfgVal(new ConfigVal(IpctSt::kIpcStUint32, sib_count));
      }
      break;
    case UNC_OP_READ_SIBLING:
      db_result = dmi->GetSiblingRecords(dt_type, tbl_index,
                                         (uint16_t) sibling_count,
                                         dal_bind_info, &dal_cursor_handle);
      break;
    case UNC_OP_READ_SIBLING_BEGIN:
      db_result = dmi->GetMultipleRecords(dt_type, tbl_index,
                                          (uint16_t) sibling_count,
                                          dal_bind_info,
                                          &dal_cursor_handle);
      break;
    case UNC_OP_READ:
      if (dbop.readop & kOpReadMultiple) {
        db_result = dmi->GetMultipleRecords(dt_type,
                                            tbl_index,
                                            sibling_count,
                                            dal_bind_info,
                                            &dal_cursor_handle);
      } else {
        db_result = dmi->GetSingleRecord(dt_type, tbl_index, dal_bind_info);
      }
      break;
    default:
      DELETE_IF_NOT_NULL(dal_bind_info);
      return UPLL_RC_ERR_GENERIC;
  }
  result_code = DalToUpllResCode(db_result);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_DEBUG("Returning %d", result_code);
    delete dal_bind_info;
    return result_code;
  }
  if (dbop.readop & kOpReadMultiple) {
    uint32_t count = 0;
    uint32_t nrec_read = 0;
    ConfigKeyVal *end_resp = NULL;
    while ((!sibling_count) || (count < sibling_count)) {
      db_result = dmi->GetNextRecord(dal_cursor_handle);
      result_code = DalToUpllResCode(db_result);
      if (result_code != UPLL_RC_SUCCESS) {
        if (result_code == UPLL_RC_ERR_NO_SUCH_INSTANCE) {
          count = (op == UNC_OP_READ)?nrec_read:count;
          result_code = (count) ? UPLL_RC_SUCCESS : result_code;
          sibling_count = count;
        }
        break;
      }
      ConfigKeyVal *prev_key = tkey;
      tkey = NULL;
      result_code = DupConfigKeyVal(tkey, ikey, tbl);
      if (result_code != UPLL_RC_SUCCESS) {
        UPLL_LOG_DEBUG("Dup failed error %d", result_code);
        delete dal_bind_info;
        DELETE_IF_NOT_NULL(end_resp);
        return result_code;
      }
      if (!end_resp)
        end_resp = tkey;
      else {
        prev_key->AppendCfgKeyVal(tkey);
      }
      if (op != UNC_OP_READ) count++; else nrec_read++;
    }
    if (result_code == UPLL_RC_SUCCESS) {
      if (end_resp)
        ikey->ResetWith(end_resp);
      UPLL_LOG_DEBUG(" sibling_count %d count %d operation %d response %s",
                     sibling_count, count, op, (ikey->ToStrAll()).c_str());
    }
    dmi->CloseCursor(dal_cursor_handle);
    DELETE_IF_NOT_NULL(end_resp);
  }
  if (dal_bind_info) delete dal_bind_info;
  return result_code;
}

upll_rc_t MoMgrImpl::UpdateConfigDB(ConfigKeyVal *ikey,
                                    upll_keytype_datatype_t dt_type,
                                    unc_keytype_operation_t op,
                                    DalDmlIntf *dmi,
                                    MoMgrTables tbl) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UpdateConfigDB(ikey, dt_type, op, dmi, NULL, tbl);

  return result_code;
}

upll_rc_t MoMgrImpl::UpdateConfigDB(ConfigKeyVal *ikey,
                                    upll_keytype_datatype_t dt_type,
                                    unc_keytype_operation_t op,
                                    DalDmlIntf *dmi,
                                    DbSubOp *pdbop,
                                    MoMgrTables tbl) {
  UPLL_FUNC_TRACE;
  const uudst::kDalTableIndex tbl_index = GetTable(tbl, dt_type);
  if (tbl_index >= uudst::kDalNumTables) {
    UPLL_LOG_DEBUG(" Invalid Table index - %d", tbl_index);
    return UPLL_RC_ERR_GENERIC;
  }

  bool exists = false;
  DalBindInfo *dal_bind_info = new DalBindInfo(tbl_index);
  upll_rc_t result_code;
  DbSubOp dbop = { kOpReadExist, kOpMatchNone, kOpInOutFlag | kOpInOutCtrlr
    | kOpInOutDomain };
  if (pdbop == NULL) {
    if (op == UNC_OP_DELETE) dbop.inoutop = kOpInOutNone;
    if (op != UNC_OP_CREATE) {
      if ((tbl == RENAMETBL) || (tbl == CTRLRTBL)) {
        dbop.matchop = kOpMatchCtrlr | kOpMatchDomain;
        dbop.inoutop = kOpInOutFlag | kOpInOutCs;
      }
      if (op == UNC_OP_UPDATE) {
        if  (dt_type == UPLL_DT_CANDIDATE) {
          dbop.inoutop = kOpInOutCs;
        } else if (dt_type == UPLL_DT_RUNNING) {
          dbop.inoutop |= kOpInOutCs;
        } else if (dt_type == UPLL_DT_AUDIT) {
          dbop.inoutop = kOpInOutFlag;
        }
      }
    } else {
      if (dt_type != UPLL_DT_CANDIDATE || tbl == CTRLRTBL)
        if (dt_type != UPLL_DT_AUDIT)
          dbop.inoutop |= kOpInOutCs;
    }
    pdbop = &dbop;
  }
  result_code = BindAttr(dal_bind_info, ikey, op, dt_type, *pdbop, tbl);
  if (result_code != UPLL_RC_SUCCESS) {
    if (dal_bind_info) delete dal_bind_info;
    return result_code;
  }
  dt_type = (dt_type == UPLL_DT_STATE) ? UPLL_DT_RUNNING : dt_type;
  switch (op) {
    case UNC_OP_CREATE:
      UPLL_LOG_TRACE("Dbop %s dt_type %d CREATE %d",
                     (ikey->ToStrAll()).c_str(), dt_type,
                     tbl_index);
      result_code = DalToUpllResCode(
          dmi->CreateRecord(dt_type, tbl_index, dal_bind_info));
      break;
    case UNC_OP_DELETE:
      UPLL_LOG_TRACE("Dbop %s dt_type %d DELETE  %d",
                     (ikey->ToStrAll()).c_str(), dt_type,
                     tbl_index);
      result_code = DalToUpllResCode(
          dmi->DeleteRecords(dt_type, tbl_index, dal_bind_info));
      break;
    case UNC_OP_UPDATE:
      UPLL_LOG_TRACE("Dbop %s dt_type %d UPD  %d",
                     (ikey->ToStrAll()).c_str(), dt_type,
                     tbl_index);
      result_code = DalToUpllResCode(
          dmi->UpdateRecords(dt_type, tbl_index, dal_bind_info));
      break;
    case UNC_OP_READ:
      UPLL_LOG_TRACE("Dbop %s dt_type %d EXISTS  %d",
                     (ikey->ToStrAll()).c_str(), dt_type,
                     tbl_index);
      result_code = DalToUpllResCode(
          dmi->RecordExists(dt_type, tbl_index, dal_bind_info, &exists));
      if (result_code == UPLL_RC_SUCCESS) {
        if (exists)
          result_code = UPLL_RC_ERR_INSTANCE_EXISTS;
        else
          result_code = UPLL_RC_ERR_NO_SUCH_INSTANCE;
      }
      break;
    default:
      break;
  }
  delete dal_bind_info;

  return result_code;
}

upll_rc_t MoMgrImpl::BindAttr(DalBindInfo *db_info,
                              ConfigKeyVal *&req,
                              unc_keytype_operation_t op,
                              upll_keytype_datatype_t dt_type,
                              DbSubOp dbop,
                              MoMgrTables tbl) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  ConfigVal *ck_val = (req) ? req->get_cfg_val() : NULL;
  void *tkey = (req) ? req->get_key() : NULL;
  void *tval = NULL, *sval = NULL;
  void *p = NULL;
  BindInfo *binfo;
  int nattr;
  uint8_t *valid = NULL, *valid_st = NULL;
  key_user_data_t *tuser_data = NULL;

  if ((req == NULL) || (tkey == NULL)) {
    UPLL_LOG_DEBUG("NULL input parameters");
    return UPLL_RC_ERR_GENERIC;
  }
  UPLL_LOG_TRACE("Valid Falg is true ConfigKeyVal %s",
                 (req->ToStrAll()).c_str());
  // COV: FORWARD NULL
  if (!GetBindInfo(tbl, dt_type, binfo, nattr)) return UPLL_RC_ERR_GENERIC;
  tuser_data = reinterpret_cast<key_user_data_t *>(req->get_user_data());
  switch (op) {
    case UNC_OP_READ:
    case UNC_OP_READ_SIBLING:
    case UNC_OP_READ_SIBLING_BEGIN:
    case UNC_OP_READ_SIBLING_COUNT:
      if (dbop.readop & ~(kOpReadExist | kOpReadCount)) {
        if (ck_val == NULL) {
          AllocVal(ck_val, dt_type, tbl);
          if (!ck_val) return UPLL_RC_ERR_GENERIC;
          req->AppendCfgVal(ck_val);
        } else if ((dt_type == UPLL_DT_STATE) &&
                   (ck_val->get_next_cfg_val() == NULL) && tbl != RENAMETBL) {
          ConfigVal *ck_val1 = NULL;
          AllocVal(ck_val1, dt_type, tbl);
          const pfc_ipcstdef_t *st_def = IpctSt::GetIpcStdef(
              ck_val->get_st_num());
          if (st_def) {
            memcpy(ck_val1->get_val(), ck_val->get_val(), st_def->ist_size);
            req->SetCfgVal(ck_val1);
            ck_val = ck_val1;
          } else {
            delete ck_val1;
          }
        }
      }
      if ((!tuser_data) && (dbop.readop & ~kOpReadCount)
          && ((dbop.inoutop & (kOpInOutFlag | kOpInOutCtrlr | kOpInOutDomain))
              || (dbop.readop & kOpReadDiff))) {
        GET_USER_DATA(req);
        tuser_data = reinterpret_cast<key_user_data_t *>(req->get_user_data());
      }
      /* fall through */
    case UNC_OP_UPDATE:
      if (dbop.matchop & kOpMatchCtrlr) {
        uint8_t *ctrlr = NULL;
        GET_USER_DATA_CTRLR(req, ctrlr);
        if (!ctrlr) {
          UPLL_LOG_DEBUG("Invalid Controller");
          return UPLL_RC_ERR_GENERIC;
        }
      } else if (dbop.matchop & kOpMatchDomain) {
        uint8_t *dom = NULL;
        GET_USER_DATA_DOMAIN(req, dom);
        if (!dom) {
          UPLL_LOG_DEBUG("Invalid Domain");
          return UPLL_RC_ERR_GENERIC;
        }
      }
      /* fall through intended */
    case UNC_OP_CREATE:
      if (ck_val) {
        tval = ck_val->get_val();
      }
      if (dt_type == UPLL_DT_STATE) {
        ConfigVal *nval = (ck_val)?ck_val->get_next_cfg_val():NULL;
        sval = (nval) ? nval->get_val() : NULL;
        if (nval && (sval == NULL)) return UPLL_RC_ERR_GENERIC;
      }
      break;
    default:
      break;
  }
  for (int i = 0; i < nattr; i++) {
    uint64_t indx = binfo[i].index;
    BindStructTypes attr_type = binfo[i].struct_type;
    UPLL_LOG_TRACE(" the attr_type %x number %d", binfo[i].struct_type, i);
    if (attr_type == CFG_KEY) {
      p = reinterpret_cast<void *>(reinterpret_cast<char *>(tkey)
                                   + binfo[i].offset);
      UPLL_LOG_TRACE(" key struct %d tkey %p p %p", attr_type, tkey, p);
      switch (op) {
        case UNC_OP_CREATE:
          UPLL_LOG_TRACE(" Bind input Key %"PFC_PFMT_u64" p %p", indx, p);
          db_info->BindInput(indx, binfo[i].app_data_type, binfo[i].array_size,
                             p);
          break;
        case UNC_OP_UPDATE:
          if (IsValidKey(tkey, indx)) {
            UPLL_LOG_TRACE("tkey %p bind match UPD p %p", tkey, p);
            db_info->BindMatch(indx, binfo[i].app_data_type,
                               binfo[i].array_size, p);
          }
          break;
        case UNC_OP_DELETE:
          if (IsValidKey(tkey, indx)) {
            UPLL_LOG_TRACE("tkey %p bind match DEL p %p", tkey, p);
            db_info->BindMatch(indx, binfo[i].app_data_type,
                               binfo[i].array_size, p);
          }
          break;
        case UNC_OP_READ:
        case UNC_OP_READ_SIBLING:
        case UNC_OP_READ_SIBLING_BEGIN:
        case UNC_OP_READ_SIBLING_COUNT:
          if ((dbop.readop & kOpReadSingle) || (dbop.readop & kOpReadExist)
              || (dbop.readop & kOpReadMultiple) ||
              (dbop.readop & kOpReadCount))  {
            if (IsValidKey(tkey, indx)) {
              UPLL_LOG_TRACE("tkey %p bind match READ p %p", tkey, p);
              db_info->BindMatch(indx, binfo[i].app_data_type,
                                 binfo[i].array_size, p);
              if (dbop.readop & kOpReadMultiple) {
                UPLL_LOG_TRACE("tkey %p bind output READ p %p", tkey, p);
                db_info->BindOutput(indx, binfo[i].app_data_type,
                                    binfo[i].array_size, p);
              }
            } else {
              UPLL_LOG_TRACE("tkey %p bind output READ p %p", tkey, p);
              db_info->BindOutput(indx, binfo[i].app_data_type,
                                  binfo[i].array_size, p);
            }
          } else if (dbop.readop & kOpReadDiff) {
            UPLL_LOG_TRACE("tkey %p DIFF match/output p %p", tkey, p);
            db_info->BindMatch(indx, binfo[i].app_data_type,
                               binfo[i].array_size, p);
            db_info->BindOutput(indx, binfo[i].app_data_type,
                                binfo[i].array_size, p);
          }
          break;
        default:
          break;
      }
    } else if (tuser_data
               && ((attr_type == CK_VAL) || (attr_type == CK_VAL2))) {
      if (attr_type == CK_VAL2) {
        if (req->get_cfg_val()) {
          GET_USER_DATA(req->get_cfg_val());
          tuser_data =
              reinterpret_cast<key_user_data *>
              (req->get_cfg_val()->get_user_data());
        } else
          tuser_data = NULL;
      } else {
        tuser_data = reinterpret_cast<key_user_data_t *>(req->get_user_data());
      }
      if (!tuser_data) {
        UPLL_LOG_DEBUG("null tuser_data");
        continue;
      }
      p = reinterpret_cast<void *>(reinterpret_cast<char *>(tuser_data)
                                   + binfo[i].offset);
      bool par_flag = false, par_ctrlr = false, par_dom = false;
      if (binfo[i].offset == offsetof(key_user_data_t, flags))
        par_flag = true;
      else if (binfo[i].offset == offsetof(key_user_data_t, ctrlr_id))
        par_ctrlr = true;
      else if (binfo[i].offset == offsetof(key_user_data_t, domain_id))
        par_dom = true;
      switch (op) {
        case UNC_OP_CREATE:
          if ((par_ctrlr && (dbop.inoutop & kOpInOutCtrlr)) ||
              (par_dom && (dbop.inoutop & kOpInOutDomain))  ||
              (par_flag && (dbop.inoutop & kOpInOutFlag))) {
            UPLL_LOG_TRACE("CR bind input Cntrlr/Domain/Flag %p", p);
            db_info->BindInput(indx, binfo[i].app_data_type,
                               binfo[i].array_size, p);
          }
          break;
        case UNC_OP_UPDATE:
          if ((par_ctrlr && (dbop.matchop & kOpMatchCtrlr)) ||
              (par_dom && (dbop.matchop & kOpMatchDomain))  ||
              (par_flag && (dbop.matchop & kOpMatchFlag))) {
            UPLL_LOG_TRACE("UPD bind match flag/Cntrlr %p ", p);
            db_info->BindMatch(indx, binfo[i].app_data_type,
                               binfo[i].array_size, p);
          } else if ((par_ctrlr && (dbop.inoutop & kOpInOutCtrlr))
                     || (par_dom && (dbop.inoutop & kOpInOutDomain))
                     || (par_flag && (dbop.inoutop & kOpInOutFlag))) {
            UPLL_LOG_TRACE("UPD bind input flag/Cntrlr/domain %p ", p);
            db_info->BindInput(indx, binfo[i].app_data_type,
                               binfo[i].array_size, p);
          }
          break;
        case UNC_OP_READ:
        case UNC_OP_READ_SIBLING:
        case UNC_OP_READ_SIBLING_BEGIN:
        case UNC_OP_READ_SIBLING_COUNT:
          if ((par_ctrlr && (dbop.inoutop & kOpInOutCtrlr)) ||
              (par_dom && (dbop.inoutop & kOpInOutDomain)) ||
              (par_flag && (dbop.inoutop & kOpInOutFlag))) {
            UPLL_LOG_TRACE("RD bind output flag/Cntrlr/domain %p", p);
            db_info->BindOutput(indx, binfo[i].app_data_type,
                                binfo[i].array_size, p);
          }
          /* fall through intended */
        case UNC_OP_DELETE:
          if ((par_ctrlr && (dbop.matchop & kOpMatchCtrlr)) ||
              (par_dom && (dbop.matchop & kOpMatchDomain)) ||
              (par_flag && (dbop.matchop & kOpMatchFlag))) {
            UPLL_LOG_TRACE("RD/DEL bind match flag/Cntrlr/domain %p", p);
            db_info->BindMatch(indx, binfo[i].app_data_type,
                               binfo[i].array_size, p);
          }
          break;
        default:
          // do nothing
          break;
      }

    } else if (tval && (attr_type != ST_VAL) && (attr_type != ST_META_VAL)) {
#if 1
      if (attr_type == CFG_DEF_VAL) {
        attr_type = (dbop.readop & kOpReadDiffUpd)?attr_type:CFG_META_VAL;
        UPLL_LOG_DEBUG("ATTR: attr_type %d readop %d op %d\n",
                       attr_type,
                       dbop.readop,
                       op);
      }
#endif
      if (op == UNC_OP_DELETE) continue;
      if (dt_type == UPLL_DT_STATE) {
#if 0
        attr_type = (attr_type == CFG_ST_VAL)?CFG_VAL:
            ((attr_type == CFG_ST_META_VAL)?CFG_META_VAL:attr_type);
#else
        // bind down count only for output and not for match
        if (attr_type == CFG_ST_VAL) {
          attr_type = (dbop.readop & kOpReadDiffUpd)?CFG_DEF_VAL:CFG_VAL;
        } else if (attr_type == CFG_ST_META_VAL) {
          attr_type = (dbop.readop & kOpReadDiffUpd)?CFG_DEF_VAL:CFG_META_VAL;
        }
#endif
      } else if ((attr_type == CFG_ST_VAL) || (attr_type == CFG_ST_META_VAL)) {
        continue;
      }
      p = reinterpret_cast<void *>(reinterpret_cast<char *>(tval)
                                   + binfo[i].offset);
      bool valid_is_defined = false;
      if (attr_type == CFG_VAL) {
        result_code = GetValid(tval, indx, valid, dt_type, tbl);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_TRACE("Returning %d", result_code);
          return result_code;
        }
        if (!valid) {
          UPLL_LOG_TRACE(" Invalid for attr %d", i);
          switch (op) {
            case UNC_OP_CREATE:
            case UNC_OP_UPDATE:
              valid_is_defined = true;
              break;
            default:
              valid_is_defined = false;
          }
        } else if ((*valid == UNC_VF_VALID) ||
                   (*valid == UNC_VF_VALID_NO_VALUE)) {
          valid_is_defined = true;
        }
      } else if (attr_type == CFG_META_VAL) {
        if ((*(reinterpret_cast<uint8_t *>(p)) == UNC_VF_VALID)
            || (*(reinterpret_cast<uint8_t *>(p)) == UNC_VF_VALID_NO_VALUE))
          valid_is_defined = true;
      }
      switch (op) {
        case UNC_OP_CREATE:
#if 0
          if ((attr_type == CFG_META_VAL) || valid_is_defined
#else
              if ((attr_type == CFG_META_VAL) || (attr_type == CFG_VAL)
#endif
                  || ((attr_type == CS_VAL) && (dbop.inoutop & kOpInOutCs))) {
              UPLL_LOG_TRACE("tval/meta CR bind input %p p %p", tval, p);
              db_info->BindInput(indx, binfo[i].app_data_type,
                                 binfo[i].array_size,
                                 reinterpret_cast<void *>(p));
              }
              break;
              case UNC_OP_UPDATE:
#if 0
              if ((attr_type == CFG_META_VAL)
                  || ((attr_type == CS_VAL) && (dbop.matchop & kOpMatchCs))) {
              UPLL_LOG_TRACE("tval/meta UP bind match %p p %p", tval, p);
              db_info->BindMatch(indx, binfo[i].app_data_type,
                                 binfo[i].array_size,
                                 reinterpret_cast<void *>(p));
              }
#endif
              if (valid_is_defined ||
                  ((attr_type == CS_VAL) && (dbop.inoutop & kOpInOutCs))) {
                UPLL_LOG_TRACE("tval/meta UP bind input %p p %p", tval, p);
                // store VALID_NO_VALUE flag in candidate as INVALID
                if ((attr_type == CFG_META_VAL) &&
                    (*(reinterpret_cast<uint8_t *>(p)) ==
                     UNC_VF_VALID_NO_VALUE)) {
                  UPLL_LOG_TRACE("Resetting VALID_NO_VALUE to INVALID %p", p);
                  *(reinterpret_cast<uint8_t *>(p)) = UNC_VF_INVALID;
                }
                db_info->BindInput(indx, binfo[i].app_data_type,
                                   binfo[i].array_size,
                                   reinterpret_cast<void *>(p));
              }
              break;
              case UNC_OP_READ:
              case UNC_OP_READ_SIBLING:
              case UNC_OP_READ_SIBLING_BEGIN:
              case UNC_OP_READ_SIBLING_COUNT:
              if (dbop.readop & ~(kOpReadDiff | kOpReadExist |
                                  kOpReadDiffUpd)) {
                if (valid_is_defined) {
                  UPLL_LOG_TRACE("tval RD bind match %p p %p", tval, p);
                  db_info->BindMatch(indx, binfo[i].app_data_type,
                                     binfo[i].array_size,
                                     reinterpret_cast<void *>(p));
                } else if ((dbop.readop &
                            (kOpReadExist | kOpReadCount)) == 0) {
                  switch (attr_type) {
                    case CS_VAL:
                      if (dbop.inoutop & kOpInOutCs) {
                        UPLL_LOG_TRACE("tvalcs RD bind output %p p %p",
                                       tval, p);
                        db_info->BindOutput(indx, binfo[i].app_data_type,
                                            binfo[i].array_size,
                                            reinterpret_cast<void *>(p));
                      }
                      break;
                    case CFG_VAL:
                    case CFG_META_VAL:
                      UPLL_LOG_TRACE("tval RD bind output %p p %p", tval, p);
                      db_info->BindOutput(indx, binfo[i].app_data_type,
                                          binfo[i].array_size,
                                          reinterpret_cast<void *>(p));
                    default:
                      break;
                  }
                }
              } else if (dbop.readop & kOpReadDiff) {
                if ((attr_type == CFG_META_VAL) || (attr_type == CFG_VAL) ||
#if 1
                    (attr_type == CFG_DEF_VAL) ||
#endif
                    ((attr_type == CS_VAL) && (dbop.inoutop & kOpInOutCs))) {
                  UPLL_LOG_TRACE("tval %d RDDiff bind output %p p %p",
                                 attr_type,
                                 tval, p);
                  db_info->BindOutput(indx, binfo[i].app_data_type,
                                      binfo[i].array_size,
                                      reinterpret_cast<void *>(p));
                }
#if 1
                if ((attr_type == CFG_META_VAL) || (attr_type == CFG_VAL) ||
#else
                    if ((attr_type == CFG_VAL) ||
#endif
                        ((attr_type == CS_VAL) &&
                         (dbop.matchop & kOpMatchCs))) {
#if 1
                    if ((dbop.readop & kOpReadDiffUpd) &&
                        (attr_type != CFG_DEF_VAL)) {
#else
                    if (dbop.readop & kOpReadDiffUpd)
#endif
                    UPLL_LOG_TRACE("tval %d RDDiff bind match %p p %p",
                                   attr_type,
                                   tval, p);
                    db_info->BindMatch(indx, binfo[i].app_data_type,
                                       binfo[i].array_size,
                                       reinterpret_cast<void *>(p));
                    }
                    }
                    }
                    default:
                    break;
                    }
    } else if (sval) {
      if (op == UNC_OP_DELETE) continue;
      bool valid_is_defined = false;
      p = reinterpret_cast<void *>(reinterpret_cast<char *>(sval)
                                   + binfo[i].offset);
#if 0
      if (attr_type == CFG_ST_VAL) {
        uint32_t val_p =  *(reinterpret_cast<uint32_t *>(p));
        attr_type = (op == UNC_OP_UPDATE)?
            ((val_p != INVALID_MATCH_VALUE)?ST_VAL:attr_type):ST_VAL;
      }
#endif
      if (attr_type == ST_VAL) {
        result_code = GetValid(sval, indx, valid_st, dt_type, tbl);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_TRACE("Returning %d", result_code);
          return result_code;
        }
        if (!valid_st) {
          switch (op) {
            case UNC_OP_CREATE:
            case UNC_OP_UPDATE:
              valid_is_defined = true;
              break;
            default:
              valid_is_defined = false;
          }
        }
        else if ((*(reinterpret_cast<uint8_t *>(valid_st)) == UNC_VF_VALID) ||
                 (*(reinterpret_cast<uint8_t *>(valid_st)) ==
                  UNC_VF_VALID_NO_VALUE))
          valid_is_defined = true;
        UPLL_LOG_TRACE(" The ST_VAL valid flag is %d", valid_is_defined);
      } else if (attr_type == ST_META_VAL) {
        if ((*(reinterpret_cast<uint8_t *>(p)) == UNC_VF_VALID)
            || (*(reinterpret_cast<uint8_t *>(p)) == UNC_VF_VALID_NO_VALUE))
          valid_is_defined = true;
      }
      switch (op) {
        case UNC_OP_CREATE:
          if ((attr_type == ST_META_VAL) || valid_is_defined) {
            UPLL_LOG_TRACE("sval CR/UPD bind input %p p %p", sval, p);
            db_info->BindInput(indx, binfo[i].app_data_type,
                               binfo[i].array_size,
                               reinterpret_cast<void *>(p));
          }
          break;
        case UNC_OP_UPDATE:
#if 0
          if (attr_type == ST_META_VAL) {
            UPLL_LOG_TRACE("sval/meta UP bind match %p p %p", sval, p);
            db_info->BindMatch(indx, binfo[i].app_data_type,
                               binfo[i].array_size,
                               reinterpret_cast<void *>(p));
          }
#endif
          if (valid_is_defined) {
            UPLL_LOG_TRACE("sval/meta UP bind input %p p %p", sval, p);
            db_info->BindInput(indx, binfo[i].app_data_type,
                               binfo[i].array_size,
                               reinterpret_cast<void *>(p));
          }
          break;
        case UNC_OP_READ:
        case UNC_OP_READ_SIBLING:
        case UNC_OP_READ_SIBLING_BEGIN:
        case UNC_OP_READ_SIBLING_COUNT:
          if (dbop.readop & ~(kOpReadDiff | kOpReadDiffUpd | kOpReadExist)) {
            if (valid_is_defined) {
              UPLL_LOG_TRACE("sval RD bind match %p p %p", sval, p);
              db_info->BindMatch(indx, binfo[i].app_data_type,
                                 binfo[i].array_size,
                                 reinterpret_cast<void *>(p));
            } else if ((dbop.readop & (kOpReadExist | kOpReadCount)) == 0) {
              UPLL_LOG_TRACE("sval RD bind output %p p %p", sval, p);
              db_info->BindOutput(indx, binfo[i].app_data_type,
                                  binfo[i].array_size,
                                  reinterpret_cast<void *>(p));
            }
          } else if (dbop.readop & kOpReadDiff) {
            if ((attr_type == ST_META_VAL) || (attr_type == ST_VAL)) {
              UPLL_LOG_TRACE("sval %d RDDiff bind output %p p %p", attr_type,
                             sval, p);
              db_info->BindOutput(indx, binfo[i].app_data_type,
                                  binfo[i].array_size,
                                  reinterpret_cast<void *>(p));
#if 0
              if (dbop.readop & kOpReadDiffUpd) {
                UPLL_LOG_TRACE("sval %d RDDiff bind match %p p %p", attr_type,
                               sval, p);
                db_info->BindMatch(indx, binfo[i].app_data_type,
                                   binfo[i].array_size,
                                   reinterpret_cast<void *>(p));
              }
#endif
            }
          }
        default:
          break;
      }
    }
  }
  return result_code;
}

upll_rc_t MoMgrImpl::UpdateRenameKey(ConfigKeyVal *&ikey,
                                     upll_keytype_datatype_t dt_type,
                                     unc_keytype_operation_t op,
                                     DalDmlIntf *dmi, DbSubOp *pdbop,
                                     MoMgrTables tbl) {
  UPLL_FUNC_TRACE;
  const uudst::kDalTableIndex tbl_index = GetTable(tbl, dt_type);
  if (tbl_index >= uudst::kDalNumTables) {
    UPLL_LOG_DEBUG(" Invalid Table index - %d", tbl_index);
    return UPLL_RC_ERR_GENERIC;
  }
  //  cout << tbl_index << "\n";
  bool exists = false;
  DalBindInfo *dal_bind_info = new DalBindInfo(tbl_index);
  upll_rc_t result_code;
  DbSubOp dbop = { kOpReadExist, kOpMatchNone, kOpInOutFlag | kOpInOutCtrlr
    | kOpInOutDomain };
  if (pdbop == NULL) {
    if (op == UNC_OP_DELETE)
      dbop.inoutop = kOpInOutNone;
    if ((tbl == RENAMETBL) || (tbl == CTRLRTBL)) {
      dbop.matchop = kOpMatchCtrlr | kOpMatchDomain;
      dbop.inoutop = kOpInOutFlag;
    }
    pdbop = &dbop;
  }
  result_code = BindAttrRename(dal_bind_info, ikey, op, dt_type, *pdbop, tbl);
  switch (op) {
    case UNC_OP_CREATE:
      result_code = DalToUpllResCode(
          dmi->CreateRecord(dt_type, tbl_index, dal_bind_info));
      break;
    case UNC_OP_UPDATE:
      result_code = DalToUpllResCode(
          dmi->UpdateRecords(dt_type, tbl_index, dal_bind_info));
      break;
    case UNC_OP_READ:
      result_code = DalToUpllResCode(
          dmi->RecordExists(dt_type, tbl_index, dal_bind_info, &exists));
      if (result_code == UPLL_RC_SUCCESS) {
        if (exists)
          result_code = UPLL_RC_ERR_INSTANCE_EXISTS;
        else
          result_code = UPLL_RC_ERR_NO_SUCH_INSTANCE;
      }
      break;
    default:
      break;
  }
  delete dal_bind_info;

  return result_code;
}

upll_rc_t MoMgrImpl::BindAttrRename(DalBindInfo *db_info,
                                    ConfigKeyVal *&req,
                                    unc_keytype_operation_t op,
                                    upll_keytype_datatype_t dt_type,
                                    DbSubOp dbop,
                                    MoMgrTables tbl) {
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  void *tkey = (req) ? req->get_key() : NULL;
  void *p = NULL;
  BindInfo *binfo = NULL;
  int nattr;
  uint64_t indx;
  key_user_data_t *tuser_data;

  if ((req == NULL) || (tkey == NULL)) {
    UPLL_LOG_DEBUG("Input is NULL");
    return UPLL_RC_ERR_GENERIC;
  }
  if (!GetRenameKeyBindInfo(req->get_key_type(), binfo, nattr, tbl)) {
    UPLL_LOG_DEBUG("GetRenameKeyBindInfo Not available for the keytype %d"
                   "For the Table %d", req->get_key_type(), tbl);
    return UPLL_RC_ERR_GENERIC;
  }
  UPLL_LOG_TRACE("The nAttribute %d", nattr);
  tuser_data = reinterpret_cast<key_user_data_t *>(req->get_user_data());
  for (int i = 0; i < nattr; i++) {
    UPLL_LOG_TRACE("The If condition value is %d i=%d", (nattr/2), i);
    if (i == (nattr / 2)) {
      if (req->get_next_cfg_key_val()
          && (req->get_next_cfg_key_val())->get_key()) {
        tkey = (req->get_next_cfg_key_val())->get_key();
        DumpRenameInfo(req->get_next_cfg_key_val());
      }
    }
    indx = binfo[i].index;
    BindStructTypes attr_type = binfo[i].struct_type;
    UPLL_LOG_TRACE("the attr_type %d attr number %d", binfo[i].struct_type,
                   i);
    p = reinterpret_cast<void *>(reinterpret_cast<char *>(tkey)
                                 + binfo[i].offset);
    UPLL_LOG_TRACE("key struct %d tkey %p p %p", attr_type, tkey, p);
    if (CFG_INPUT_KEY == attr_type || CFG_MATCH_KEY == attr_type) {
      switch (op) {
        case UNC_OP_CREATE:
#if 0
          if (!IsValidKey(tkey, indx))  {
            UPLL_LOG_TRACE("Given Key is Invalid %s",
                           (req->ToStrAll()).c_str());
            return UPLL_RC_ERR_GENERIC;
          }
#endif
          UPLL_LOG_TRACE(" Bind input Key %"PFC_PFMT_u64" p %p", indx,
                         reinterpret_cast<char*>(p));
          db_info->BindInput(indx, binfo[i].app_data_type, binfo[i].array_size,
                             p);
          break;
        case UNC_OP_UPDATE:
          UPLL_LOG_TRACE("Validate the Key in Update");
          //          if (IsValidKey(tkey, indx)) {
          switch (attr_type) {
            case CFG_INPUT_KEY:
              UPLL_LOG_TRACE("tkey %p bindinput %p", tkey,
                             reinterpret_cast<char*>(p));
              db_info->BindInput(indx, binfo[i].app_data_type,
                                 binfo[i].array_size, p);
              break;
            case CFG_MATCH_KEY:
              UPLL_LOG_TRACE("tkey %p bindmatch %p", tkey,
                             reinterpret_cast<char*>(p));
              db_info->BindMatch(indx, binfo[i].app_data_type,
                                 binfo[i].array_size, p);
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }
      }
      if (tuser_data && attr_type == CK_VAL) {
        p = reinterpret_cast<void *>(reinterpret_cast<char *>(tuser_data)
                                     + binfo[i].offset);
        switch (op) {
          case UNC_OP_CREATE:
            if ((dbop.inoutop & (kOpInOutCtrlr | kOpInOutDomain))) {
              UPLL_LOG_TRACE("CR bind input Cntrlr/Flag %p", p);
              db_info->BindInput(indx, binfo[i].app_data_type,
                                 binfo[i].array_size, p);
            }
            break;
          case UNC_OP_UPDATE:
            if ((dbop.matchop & (kOpMatchCtrlr | kOpMatchDomain))
                || (dbop.matchop & kOpMatchFlag)) {
              UPLL_LOG_TRACE("UPD bind match Cntrlr/Flag %p", p);
              db_info->BindMatch(indx, binfo[i].app_data_type,
                                 binfo[i].array_size, p);
            } else if ((dbop.inoutop & (kOpInOutCtrlr | kOpInOutDomain))
                       || (dbop.inoutop & kOpInOutFlag)) {
              UPLL_LOG_TRACE("UPD bind input Cntrlr/Flag %p", p);
              db_info->BindInput(indx, binfo[i].app_data_type,
                                 binfo[i].array_size, p);
            }
            break;
          default:
            break;
        }
      }
    }
    return result_code;
  }

  upll_rc_t MoMgrImpl::BindStartup(DalBindInfo *db_info,
                                   upll_keytype_datatype_t dt_type,
                                   MoMgrTables tbl) {
    UPLL_FUNC_TRACE;
    int nattr;
    BindInfo *binfo;
    unc_keytype_configstatus_t cs_val = UNC_CS_NOT_APPLIED;
    if (!GetBindInfo(tbl, dt_type, binfo, nattr))
      return UPLL_RC_ERR_GENERIC;
    for (int i = 0; i < nattr; i++) {
      uint64_t indx = binfo[i].index;
      BindStructTypes attr_type = binfo[i].struct_type;
      if (attr_type == CS_VAL) {
        db_info->BindInput(indx, binfo[i].app_data_type,
                           binfo[i].array_size, &cs_val);
      }
    }
    return UPLL_RC_SUCCESS;
  }

  // Binding Dummy pointers for matching
  // This is currently specific for CheckRecordsIdentical API.
  upll_rc_t MoMgrImpl::BindCandidateDirty(DalBindInfo *db_info,
                                          upll_keytype_datatype_t dt_type,
                                          MoMgrTables tbl,
                                          const uudst::kDalTableIndex index) {
    UPLL_FUNC_TRACE;
    int nattr;
    BindInfo *binfo;

    void *dummy = malloc(sizeof(uint16_t)); /* dummy pointer */
    if (dummy == NULL) {
      throw new std::bad_alloc;
    }
    memset(dummy, 0, sizeof(uint16_t));

    if (!GetBindInfo(tbl, dt_type, binfo, nattr)) {
      free(dummy);
      return UPLL_RC_ERR_GENERIC;
    }

    for (int i = 0; i < nattr; i++) {
      uint64_t indx = binfo[i].index;
      BindStructTypes attr_type = binfo[i].struct_type;
      if (attr_type != CS_VAL && attr_type != ST_VAL &&
          attr_type != ST_META_VAL && ((
                  (attr_type == CFG_KEY)
                  || (uudst::kDbiVtnCtrlrTbl != index))
              || ((CK_VAL == attr_type) &&
                  (uudst::kDbiVtnCtrlrTbl == index)))) {
        UPLL_LOG_TRACE("Bind for attr type %d", attr_type);
        db_info->BindMatch(indx, binfo[i].app_data_type,
                           binfo[i].array_size, dummy);
      }
    }

    free(dummy);
    return UPLL_RC_SUCCESS;
  }

  upll_rc_t MoMgrImpl::TxCopyRenameTableFromCandidateToRunning(
      unc_key_type_t key_type,
      unc_keytype_operation_t op,
      DalDmlIntf* dmi) {
    UPLL_FUNC_TRACE;
    upll_rc_t result_code = UPLL_RC_SUCCESS;
    ConfigKeyVal *can_ckv = NULL, *run_ckv = NULL;
    DalCursor *cfg1_cursor = NULL;
    DalResultCode db_result = uud::kDalRcSuccess;
    if (op == UNC_OP_UPDATE) {
      UPLL_LOG_TRACE("No action is performed for Update");
      return UPLL_RC_SUCCESS;
    }
    result_code = DiffConfigDB(UPLL_DT_CANDIDATE, UPLL_DT_RUNNING, op,
                               can_ckv, run_ckv, &cfg1_cursor, dmi, NULL,
                               RENAMETBL, true);
    while (result_code == UPLL_RC_SUCCESS) {
      db_result = dmi->GetNextRecord(cfg1_cursor);
      result_code = DalToUpllResCode(db_result);
      if (result_code != UPLL_RC_SUCCESS)
        break;
      /* VRT and VBR sharing the same table so need not use
       * VRT key type here */
      switch (key_type) {
        case UNC_KT_VTN: {
          val_rename_vtn *ren_val = static_cast<val_rename_vtn *>(
              GetVal(can_ckv));
          ren_val->valid[UPLL_IDX_NEW_NAME_RVTN] = UNC_VF_VALID;
        }
        break;
        case UNC_KT_VBRIDGE:
        case UNC_KT_VLINK: {
          val_rename_vnode *ren_val = static_cast<val_rename_vnode *>(
              GetVal(can_ckv));
          ren_val->valid[UPLL_CTRLR_VTN_NAME_VALID] = UNC_VF_VALID;
          ren_val->valid[UPLL_CTRLR_VNODE_NAME_VALID] = UNC_VF_VALID;
        }
        break;
        case UNC_KT_POLICING_PROFILE: {
          val_rename_policingprofile_t *ren_val = static_cast
              <val_rename_policingprofile_t *>(GetVal(can_ckv));
          ren_val->valid[UPLL_IDX_RENAME_PROFILE_RPP] = UNC_VF_VALID;
        }
        break;
        case UNC_KT_FLOWLIST: {
          val_rename_flowlist_t *ren_val = static_cast
              <val_rename_flowlist_t *>(GetVal(can_ckv));
          ren_val->valid[UPLL_IDX_RENAME_FLOWLIST_RFL] = UNC_VF_VALID;
        }
        break;
        default:
        UPLL_LOG_DEBUG("No special operation for %u", key_type);
        break;
      }
      // Copy Rename Table Info into Running
      result_code = UpdateConfigDB(can_ckv,
                                   UPLL_DT_RUNNING,
                                   op,
                                   dmi,
                                   RENAMETBL);
      if (result_code != UPLL_RC_SUCCESS) {
        delete can_ckv; can_ckv = NULL;
        DELETE_IF_NOT_NULL(run_ckv);
        if (cfg1_cursor)
          dmi->CloseCursor(cfg1_cursor, true);
        UPLL_LOG_DEBUG("Returning error %d", result_code);
        return UPLL_RC_ERR_GENERIC;
      }
    }
    if (cfg1_cursor)
      dmi->CloseCursor(cfg1_cursor, true);
    DELETE_IF_NOT_NULL(can_ckv);
    DELETE_IF_NOT_NULL(run_ckv);
    result_code = (result_code == UPLL_RC_ERR_NO_SUCH_INSTANCE)?
        UPLL_RC_SUCCESS : result_code;
    return result_code;
  }

#if 0
  template<typename T1, typename T2>
upll_rc_t MoMgrImpl::GetCkvWithOperSt(ConfigKeyVal *&ck_vn,
                                      unc_key_type_t ktype,
                                      DalDmlIntf     *dmi) {
  if (ck_vn != NULL) return UPLL_RC_ERR_GENERIC;
  ConfigVal *cval = NULL;
  MoMgrImpl *mgr = NULL;
  upll_rc_t result_code = AllocVal(cval, UPLL_DT_STATE, MAINTBL);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_DEBUG("Returning error %d", result_code);
    return result_code;
  }

  /* initialize vnode st */
  T2 * vnode_st = reinterpret_cast<T2 *>
      (cval->get_next_cfg_val()->get_val());
  if (!vnode_st) {
    delete cval;
    UPLL_LOG_DEBUG("Invalid param");
    return UPLL_RC_ERR_GENERIC;
  }
  T1 *vn_st = reinterpret_cast<T1 *>(vnode_st);
  vn_st->valid[UPLL_IDX_OPER_STATUS_VBRIS] = UNC_VF_VALID;
  vn_st->oper_status = UPLL_OPER_STATUS_UNINIT;

  /* Create Vnode If child */
  switch (ktype) {
    case UNC_KT_VTN:
    case UNC_KT_VLINK:
      mgr = reinterpret_cast<MoMgrImpl *>(const_cast<MoManager *>
                                          (GetMoManager(ktype)));
      break;
    case UNC_KT_VBRIDGE:
      mgr = reinterpret_cast<MoMgrImpl *>(const_cast<MoManager *>
                                          (GetMoManager(UNC_KT_VBR_IF)));
      break;
    case UNC_KT_VROUTER:
      mgr = reinterpret_cast<MoMgrImpl *>(const_cast<MoManager *>
                                          (GetMoManager(UNC_KT_VRT_IF)));
      break;
    case UNC_KT_VTEP:
      mgr = reinterpret_cast<MoMgrImpl *>(const_cast<MoManager *>
                                          (GetMoManager(UNC_KT_VTEP_IF)));
      break;
    case UNC_KT_VTUNNEL:
      mgr = reinterpret_cast<MoMgrImpl *>(const_cast<MoManager *>
                                          (GetMoManager(UNC_KT_VTUNNEL_IF)));
      break;
    default:
      UPLL_LOG_DEBUG("Unsupported operation on keytype %d", ktype);
      return UPLL_RC_ERR_GENERIC;
  }
  result_code = mgr->GetChildConfigKey(ck_vn, NULL);
  if (UPLL_RC_SUCCESS != result_code || ck_vn == NULL)  {
    delete cval;
    if (ck_vn) delete ck_vn;
    UPLL_LOG_DEBUG("GetChildConfigKey Failed %d", result_code);
    return result_code;
  }
  ck_vn->AppendCfgVal(cval);

  /* Reading the Vnode Table and Check the Operstatus is unknown
   * for any one of the vnode if */
  DbSubOp dbop = { kOpReadMultiple, kOpMatchNone, kOpInOutFlag |
    kOpInOutCtrlr | kOpInOutDomain };
  result_code = mgr->ReadConfigDB(ck_vn, UPLL_DT_STATE, UNC_OP_READ,
                                  dbop, dmi, MAINTBL);
  if (UPLL_RC_SUCCESS != result_code) {
    result_code = (result_code == UPLL_RC_ERR_NO_SUCH_INSTANCE)?
        UPLL_RC_SUCCESS : result_code;
    UPLL_LOG_DEBUG("Returning %d", result_code);
    if (ck_vn) delete ck_vn;
    return result_code;
  }
  return UPLL_RC_SUCCESS;
}

template upll_rc_t
MoMgrImpl::GetCkvWithOperSt<val_vlink_st_t, val_db_vlink_st_t> (
    ConfigKeyVal *&ck_vn,
    unc_key_type_t ktype,
    DalDmlIntf   *dmi);
template upll_rc_t
MoMgrImpl::GetCkvWithOperSt<val_vbr_if_st_t, val_db_vbr_if_st_t> (
    ConfigKeyVal *&ck_vn,
    unc_key_type_t ktype,
    DalDmlIntf   *dmi);
template upll_rc_t
MoMgrImpl::GetCkvWithOperSt<val_vrt_if_st_t, val_db_vrt_if_st_t> (
    ConfigKeyVal *&ck_vn,
    unc_key_type_t ktype,
    DalDmlIntf   *dmi);
template upll_rc_t
MoMgrImpl::GetCkvWithOperSt<val_vtunnel_if_st_t, val_db_vtunnel_if_st_t> (
    ConfigKeyVal *&ck_vn,
    unc_key_type_t ktype,
    DalDmlIntf   *dmi);
template upll_rc_t
MoMgrImpl::GetCkvWithOperSt<val_vtep_if_st_t, val_db_vtep_if_st_t> (
    ConfigKeyVal *&ck_vn,
    unc_key_type_t ktype,
    DalDmlIntf   *dmi);
template upll_rc_t
MoMgrImpl::GetCkvWithOperSt<val_vtn_st_t, val_db_vtn_st_t> (
    ConfigKeyVal *&ck_vn,
    unc_key_type_t ktype,
    DalDmlIntf   *dmi);
#else

upll_rc_t MoMgrImpl::GetUninitOperState(ConfigKeyVal *&ck_vn,
                                        DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  result_code = GetCkvUninit(ck_vn, NULL, dmi);
  if (result_code != UPLL_RC_SUCCESS) {
    UPLL_LOG_DEBUG("Returning error %d\n", result_code);
    return result_code;
  }
  DbSubOp dbop = { kOpReadMultiple, kOpMatchNone, kOpInOutFlag |
    kOpInOutCtrlr | kOpInOutDomain };
  result_code = ReadConfigDB(ck_vn, UPLL_DT_STATE, UNC_OP_READ,
                             dbop, dmi, MAINTBL);
  if (UPLL_RC_SUCCESS != result_code) {
    result_code = (result_code == UPLL_RC_ERR_NO_SUCH_INSTANCE)?
        UPLL_RC_SUCCESS : result_code;
    UPLL_LOG_DEBUG("Returning %d", result_code);
    delete ck_vn;
    ck_vn = NULL;
  }
  return result_code;
}

upll_rc_t MoMgrImpl::GetCkvUninit(ConfigKeyVal *&ck_vn,
                                  ConfigKeyVal *ikey,
                                  DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  upll_rc_t result_code = UPLL_RC_SUCCESS;

  if (ck_vn == NULL) {
    ConfigVal *cval = NULL;
    /* Create ckv of corresponding keytype */
    result_code = GetChildConfigKey(ck_vn, ikey);
    if (UPLL_RC_SUCCESS != result_code)  {
      UPLL_LOG_DEBUG("GetChildConfigKey Failed %d", result_code);
      return result_code;
    }
    /* Allocate Memory for vnode st */
    result_code = AllocVal(cval, UPLL_DT_STATE, MAINTBL);
    if (result_code != UPLL_RC_SUCCESS) {
      UPLL_LOG_DEBUG("Returning error %d", result_code);
      DELETE_IF_NOT_NULL(ck_vn);
      return result_code;
    }
    ck_vn->AppendCfgVal(cval);
  }
  /* initialize vnode st */
  void *vnif = GetStateVal(ck_vn);
  if (!vnif) {
    UPLL_LOG_DEBUG("Invalid param\n");
    DELETE_IF_NOT_NULL(ck_vn);
    return UPLL_RC_ERR_GENERIC;
  }
  switch (ck_vn->get_key_type()) {
    case UNC_KT_VTN:
      {
        val_vtn_st *vtnst = reinterpret_cast<val_vtn_st *>(vnif);
        vtnst->valid[UPLL_IDX_OPER_STATUS_VBRIS] = UNC_VF_VALID;
        vtnst->oper_status = UPLL_OPER_STATUS_UNINIT;
        break;
      }
    case UNC_KT_VBRIDGE:
    case UNC_KT_VROUTER:
    case UNC_KT_VTUNNEL:
    case UNC_KT_VTEP:
    case UNC_KT_VLINK:
      {
        /* cast generically as vbr as all  vnode st structures
         * are the same and form the first field in the db st structure.
         */
        val_vbr_st *vnodest = reinterpret_cast<val_vbr_st *>(vnif);
        vnodest->valid[UPLL_IDX_OPER_STATUS_VBRIS] = UNC_VF_VALID;
        vnodest->oper_status = UPLL_OPER_STATUS_UNINIT;
        break;
      }
    case UNC_KT_VBR_IF:
    case UNC_KT_VRT_IF:
    case UNC_KT_VTEP_IF:
    case UNC_KT_VTUNNEL_IF:
      {
        /* cast generically as vbr_if as all  vnodeif st structures
         * are the same and form the first field in the db st structure.
         */
        val_vbr_if_st *vnifst = reinterpret_cast<val_vbr_if_st *>(vnif);
        vnifst->valid[UPLL_IDX_OPER_STATUS_VBRIS] = UNC_VF_VALID;
        vnifst->oper_status = UPLL_OPER_STATUS_UNINIT;
        break;
      }
    default:
      UPLL_LOG_DEBUG("Unsupported keytype\n");
      DELETE_IF_NOT_NULL(ck_vn);
      return UPLL_RC_ERR_GENERIC;
  }

#if 0
  /* Reading the Vnode Table and Check the Operstatus is unknown
   * for any one of the vnode if */
  DbSubOp dbop = { kOpReadExist | kOpReadMultiple, kOpMatchNone, kOpInOutFlag |
    kOpInOutCtrlr | kOpInOutDomain };
  if (PORT_MAPPED_KEYTYPE(ck_vn->get_key_type()))
    dbop.readop = kOpReadMultiple;
#endif
  return result_code;
}

upll_rc_t MoMgrImpl::BindImportDB(ConfigKeyVal *&ikey,
                                  DalBindInfo *&db_info,
                                  upll_keytype_datatype_t dt_type,
                                  MoMgrTables tbl ) {
  UPLL_FUNC_TRACE;
  int nattr = 0;
  BindInfo *binfo;
  ConfigVal *ck_val = NULL;
  key_user_data *tuser_data  = NULL;
  if (!ikey || !(ikey->get_key())) {
    UPLL_LOG_DEBUG("Input key is Empty");
    return UPLL_RC_ERR_GENERIC;
  }
  /* Allocate memeory for key user data to fetch
   * controller, domain and rename flag */
  AllocVal(ck_val, dt_type, RENAMETBL);
  if (!ck_val) return UPLL_RC_ERR_GENERIC;
  ikey->SetCfgVal(ck_val);
  void *tval = ck_val->get_val();
  if (!tval) return UPLL_RC_ERR_GENERIC;
  GET_USER_DATA(ikey);
  tuser_data = reinterpret_cast<key_user_data_t *>(ikey->get_user_data());

  if (!tuser_data) {
    UPLL_LOG_DEBUG("Memory Allocation Failed");
    return UPLL_RC_ERR_GENERIC;
  }
  void *tkey = ikey->get_key();
  void *p = NULL;

  if (!GetBindInfo(tbl, dt_type, binfo, nattr))
    return UPLL_RC_ERR_GENERIC;

  for (int i = 0; i < nattr; i++) {
    uint64_t indx = binfo[i].index;
    BindStructTypes attr_type = binfo[i].struct_type;

    UPLL_LOG_TRACE("Attribute type is %d", attr_type);
    if (CFG_KEY == attr_type) {
      p = reinterpret_cast<void *>(reinterpret_cast<char *>(tkey)
                                   + binfo[i].offset);
      UPLL_LOG_TRACE("Attribute type is %d", attr_type);
      if (IsValidKey(tkey, indx)) {
        UPLL_LOG_TRACE("Key is valid ");
        db_info->BindMatch(indx, binfo[i].app_data_type,
                           binfo[i].array_size, p);
      }
    }
    if (CK_VAL == attr_type) {
      /* For Domain and controller output */
      UPLL_LOG_TRACE("Attribute type is %d", attr_type);
      p = reinterpret_cast<void *>(reinterpret_cast<char *>(tuser_data)
                                   + binfo[i].offset);
    }
    if (CFG_VAL == attr_type) {
      UPLL_LOG_TRACE("Attribute type is %d", attr_type);
      p = reinterpret_cast<void *>(reinterpret_cast<char *>(tval)
                                   + binfo[i].offset);
    }
    if (p)
      db_info->BindOutput(indx, binfo[i].app_data_type,
                          binfo[i].array_size, p);
  }

  return UPLL_RC_SUCCESS;
}

upll_rc_t MoMgrImpl::Getvalstnum(ConfigKeyVal *&ikey,
                                 uui::IpctSt::IpcStructNum &struct_num) {
  switch (ikey->get_key_type()) {
    case UNC_KT_FLOWLIST:
      struct_num = IpctSt::kIpcStValRenameFlowlist;
      break;
    case UNC_KT_POLICING_PROFILE:
      struct_num = IpctSt::kIpcStValRenamePolicingprofile;
      break;
    case UNC_KT_VTN:
      struct_num = IpctSt::kIpcStValRenameVtn;
      break;
    case UNC_KT_VBRIDGE:
      struct_num = IpctSt::kIpcStValRenameVbr;
      break;
    case UNC_KT_VROUTER:
      struct_num = IpctSt::kIpcStValRenameVrt;
      break;
    case UNC_KT_VLINK:
      struct_num = IpctSt::kIpcStValRenameVlink;
      break;
    default:
      struct_num  = IpctSt::kIpcInvalidStNum;
      break;
  }
  return UPLL_RC_SUCCESS;
}

upll_rc_t MoMgrImpl::Swapvaltokey(ConfigKeyVal *&ikey,
                                  uint8_t rename_flag) {
  UPLL_FUNC_TRACE;
  void *rename_val = NULL;
  uint8_t temp_str[33];
  void *rename = NULL;
  uui::IpctSt::IpcStructNum struct_num  = IpctSt::kIpcInvalidStNum;
  if (!ikey || !(ikey->get_key())) {
    return UPLL_RC_ERR_GENERIC;
  }
  Getvalstnum(ikey, struct_num);
  if (rename_flag != IMPORT_READ_FAILURE) {
    switch (ikey->get_key_type()) {
      case UNC_KT_FLOWLIST:
        {
          rename = reinterpret_cast<val_rename_flowlist_t *>(
              ConfigKeyVal::Malloc(sizeof(val_rename_flowlist_t)));
          rename_val = reinterpret_cast<val_rename_flowlist_t *>(GetVal(ikey));
          if (!rename_val) {
            UPLL_LOG_DEBUG("Val is Empty");
            free(rename);
            return UPLL_RC_ERR_GENERIC;
          }
          if (!rename_flag) {
            if (!strcmp((const char *)reinterpret_cast<key_flowlist_t *>
                        (ikey->get_key())->flowlist_name,
                        (const char *)reinterpret_cast<val_rename_vtn_t*>
                        (rename_val)->new_name))
              reinterpret_cast<val_rename_flowlist_t*>
                  (rename)->valid[UPLL_CTRLR_VTN_NAME_VALID] = UNC_VF_INVALID;
          }
          else {
            reinterpret_cast<val_rename_flowlist_t*>
                (rename)->valid[UPLL_CTRLR_VTN_NAME_VALID] = UNC_VF_VALID;
          }
          /* copyt key to temp */
          uuu::upll_strncpy(temp_str, reinterpret_cast<key_flowlist_t*>
                             (ikey->get_key())->flowlist_name,
                             (kMaxLenFlowListName+1));
          /* Copy Controller name to key */
          uuu::upll_strncpy(reinterpret_cast<key_flowlist_t *>
                            (ikey->get_key())->flowlist_name,
                            reinterpret_cast<val_rename_flowlist_t* >
                            (rename_val)->flowlist_newname,
                            (kMaxLenFlowListName+1));
          /* Copy the UNC name to Val */
          uuu::upll_strncpy(reinterpret_cast<val_rename_flowlist_t*>
                             (rename)->flowlist_newname, temp_str,
                             (kMaxLenFlowListName+1));
        }
        break;
      case UNC_KT_POLICING_PROFILE:
        {
          rename = reinterpret_cast<val_rename_policingprofile_t *>(
              ConfigKeyVal::Malloc(sizeof(val_rename_policingprofile_t)));
          rename_val = reinterpret_cast
              <val_rename_policingprofile_t *>(GetVal(ikey));
          if (!rename_val) {
            UPLL_LOG_DEBUG("Val is Empty");
            free(rename);
            return UPLL_RC_ERR_GENERIC;
          }
          if (!rename_flag) {
            if (!strcmp((const char *)reinterpret_cast<key_policingprofile *>
                        (ikey->get_key())->policingprofile_name,
                        (const char *)reinterpret_cast<val_rename_vtn_t*>
                        (rename_val)->new_name))
              reinterpret_cast<val_rename_policingprofile_t*>(rename)->
                  valid[UPLL_CTRLR_VTN_NAME_VALID] = UNC_VF_INVALID;
          }
          else {
            reinterpret_cast<val_rename_policingprofile_t*>(rename)->
                valid[UPLL_CTRLR_VTN_NAME_VALID] = UNC_VF_VALID;
          }
          /* copyt key to temp */
          uuu::upll_strncpy(temp_str, reinterpret_cast<key_policingprofile_t*>
                             (ikey->get_key())->policingprofile_name,
                             (kMaxLenPolicingProfileName+1));
          /* Copy Controller name to key */
          uuu::upll_strncpy(reinterpret_cast<key_policingprofile_t *>
                            (ikey->get_key())->policingprofile_name,
                            reinterpret_cast<val_rename_policingprofile_t* >
                            (rename_val)->policingprofile_newname,
                            (kMaxLenPolicingProfileName+1));
          /* Copy the UNC name to Val */
          uuu::upll_strncpy(reinterpret_cast<val_rename_policingprofile_t*>
                             (rename)->policingprofile_newname, temp_str,
                             (kMaxLenPolicingProfileName+1));
        }

        break;
      case UNC_KT_VTN:
        {
          rename = reinterpret_cast<val_rename_vtn_t *>(
              ConfigKeyVal::Malloc(sizeof(val_rename_vtn_t)));
          rename_val = reinterpret_cast<val_rename_vtn_t *>(GetVal(ikey));
          if (!rename_val) {
            UPLL_LOG_DEBUG("Val is Empty");
            free(rename);
            return UPLL_RC_ERR_GENERIC;
          }
          if (!rename_flag) {
            if (!strcmp((const char *)reinterpret_cast<key_vtn_t *>
                        (ikey->get_key())->vtn_name,
                        (const char *)reinterpret_cast<val_rename_vtn_t*>
                        (rename_val)->new_name))
              reinterpret_cast<val_rename_vtn_t*>(rename)->
                  valid[UPLL_CTRLR_VTN_NAME_VALID] = UNC_VF_INVALID;
          }
          else {
            reinterpret_cast<val_rename_vtn_t*>(rename)->
                valid[UPLL_CTRLR_VTN_NAME_VALID] = UNC_VF_VALID;
          }
          /* copyt key to temp */
          uuu::upll_strncpy(temp_str, reinterpret_cast<key_vtn_t*>
                             (ikey->get_key())->vtn_name,
                             (kMaxLenVtnName+1));
          /* Copy Controller name to key */
          uuu::upll_strncpy(reinterpret_cast<key_vtn_t *>
                            (ikey->get_key())->vtn_name,
                            reinterpret_cast<val_rename_vtn_t* >
                            (rename_val)->new_name,
                            (kMaxLenVtnName+1));
          /* Copy the UNC name to Val */
          uuu::upll_strncpy(reinterpret_cast<val_rename_vtn_t*>
                             (rename)->new_name, temp_str,
                             (kMaxLenVtnName+1));
        }
        break;
      case UNC_KT_VBRIDGE:
      case UNC_KT_VROUTER:
      case UNC_KT_VLINK:
        {
          rename = reinterpret_cast<val_rename_vtn_t *>(
              ConfigKeyVal::Malloc(sizeof(val_rename_vtn_t)));
          rename_val = reinterpret_cast<val_rename_vnode_t *>(GetVal(ikey));
          if (!rename_val) {
            UPLL_LOG_DEBUG("Val is Empty");
            free(rename);
            return UPLL_RC_ERR_GENERIC;
          }
          if (!strcmp((const char*)reinterpret_cast<key_vbr_t *>
                      (ikey->get_key())->vbridge_name,
                      (const char *)reinterpret_cast<val_rename_vnode_t*>
                      (rename_val)->ctrlr_vnode_name)) {
            reinterpret_cast<val_rename_vtn_t*>(rename)->
                valid[UPLL_CTRLR_VTN_NAME_VALID] =
                UNC_VF_INVALID;
          }
          else {
            reinterpret_cast<val_rename_vtn_t*>(rename)->
                valid[UPLL_CTRLR_VTN_NAME_VALID] =
                UNC_VF_VALID;
          }

          /* copyt key to temp */
          uuu::upll_strncpy(temp_str, reinterpret_cast<key_vbr_t*>
                             (ikey->get_key())->vbridge_name,
                             (kMaxLenVnodeName+1));

          /* Copy Controller name to key */
          uuu::upll_strncpy(reinterpret_cast<key_vbr_t *>
                            (ikey->get_key())->vtn_key.vtn_name,
                            reinterpret_cast<val_rename_vnode_t* >
                            (rename_val)->ctrlr_vtn_name,
                            (kMaxLenVtnName+1));
          uuu::upll_strncpy(reinterpret_cast<key_vbr_t *>
                            (ikey->get_key())->vbridge_name,
                            reinterpret_cast<val_rename_vnode_t* >
                            (rename_val)->ctrlr_vnode_name,
                            (kMaxLenVnodeName+1));
          /* Copy the UNC name to Val */
          uuu::upll_strncpy(reinterpret_cast<val_rename_vtn_t*>
                            (rename)->new_name,
                             temp_str,
                             (kMaxLenVnodeName+1));
        }
        break;
      default:
        break;
    }
  }
  ikey->SetCfgVal(new ConfigVal(struct_num, rename));
  return UPLL_RC_SUCCESS;
}


upll_rc_t MoMgrImpl::SwapKey(ConfigKeyVal *&ikey,
                             uint8_t rename_flag) {
  UPLL_FUNC_TRACE;
  uui::IpctSt::IpcStructNum struct_num  = IpctSt::kIpcInvalidStNum;
  void *rename = NULL;
  UPLL_LOG_TRACE("Before Swap Key %s %d", ikey->ToStrAll().c_str(),
                 rename_flag);
  if (rename_flag) {
    Swapvaltokey(ikey, rename_flag);
  } else {
    Getvalstnum(ikey, struct_num);

    switch (ikey->get_key_type()) {
      case UNC_KT_FLOWLIST:
        rename = reinterpret_cast
            <val_rename_flowlist *>(ConfigKeyVal::Malloc
                                    (sizeof(val_rename_flowlist)));
        uuu::upll_strncpy(reinterpret_cast<val_rename_flowlist *>(rename)
                          ->flowlist_newname,
                          reinterpret_cast<key_flowlist_t*>(ikey->get_key())
                          ->flowlist_name, (kMaxLenFlowListName+1));
        break;
      case UNC_KT_POLICING_PROFILE:
        rename = reinterpret_cast
            <val_rename_policingprofile *>(
                ConfigKeyVal::Malloc(sizeof(val_rename_policingprofile)));
        uuu::upll_strncpy(reinterpret_cast<val_rename_policingprofile *>
                          (rename)->policingprofile_newname,
                          reinterpret_cast<key_policingprofile_t*>
                          (ikey->get_key())->policingprofile_name,
                          (kMaxLenPolicingProfileName+1));
        break;
      case UNC_KT_VTN:
        rename = reinterpret_cast<val_rename_vtn_t *>(
            ConfigKeyVal::Malloc(sizeof(val_rename_vtn_t)));
        uuu::upll_strncpy(reinterpret_cast<val_rename_vtn_t *>
                          (rename)->new_name,
                          reinterpret_cast<key_vtn_t*>
                          (ikey->get_key())->vtn_name,
                          (kMaxLenVtnName+1));
        break;
      case UNC_KT_VBRIDGE:
      case UNC_KT_VROUTER:
      case UNC_KT_VLINK:
        rename = reinterpret_cast<val_rename_vtn_t *>(
            ConfigKeyVal::Malloc(sizeof(val_rename_vtn_t)));
        uuu::upll_strncpy(reinterpret_cast<val_rename_vtn_t *>
                          (rename)->new_name,
                          reinterpret_cast<key_vbr_t*>
                          (ikey->get_key())->vbridge_name,
                          (kMaxLenVnodeName+1));
        break;
      default:
        return UPLL_RC_ERR_GENERIC;
        break;
    }

    reinterpret_cast<val_rename_vtn_t *>(rename)->valid
        [UPLL_CTRLR_VTN_NAME_VALID] = UNC_VF_INVALID;
    ikey->SetCfgVal(new ConfigVal(struct_num, rename));
  }
  UPLL_LOG_TRACE("AfterSwap Key %s", ikey->ToStrAll().c_str());
  return UPLL_RC_SUCCESS;
}


std::string MoMgrImpl::GetReadImportQueryString(unc_keytype_operation_t op,
                                                unc_key_type_t kt) const {
  std::string query_string = "";
  if (op == UNC_OP_READ_SIBLING) {
    switch (kt) {
      case UNC_KT_FLOWLIST:
        query_string = \
                       " (select unc_flowlist_name, ctrlr_name, ctrlr_flowlist_name from "
                       " (select unc_flowlist_name, ctrlr_name, ctrlr_flowlist_name "
                       " from im_flowlist_rename_tbl "
                       " union all "
                       "select flowlist_name, ctrlr_name, flowlist_name from "
                       "im_flowlist_ctrlr_tbl where flags & 1 = 0 ) as temp "
                       "where ctrlr_flowlist_name > ?) "
                       " order by ctrlr_flowlist_name ";
        break;
      case UNC_KT_POLICING_PROFILE:
        query_string = "(select unc_policingprofile_name, ctrlr_name, ctrlr_policingprofile_name from \
                        (select unc_policingprofile_name, ctrlr_name, ctrlr_policingprofile_name \
                         from im_policingprofile_rename_tbl union all select policingprofile_name, \
                         ctrlr_name, policingprofile_name from im_policingprofile_ctrlr_tbl \
                         where flags & 1 = 0 ) as temp where ctrlr_policingprofile_name > ?) \
                         order by ctrlr_policingprofile_name";
        break;
      case UNC_KT_VTN:
        query_string = "(select unc_vtn_name, controller_name, domain_id, ctrlr_vtn_name from \
                        (select unc_vtn_name, controller_name, domain_id, ctrlr_vtn_name \
                         from im_vtn_rename_tbl union all select vtn_name, \
                         controller_name, domain_id, vtn_name from im_vtn_ctrlr_tbl \
                         where flags & 1 = 0) as temp where ctrlr_vtn_name > ?) order by ctrlr_vtn_name";
        break;
      case UNC_KT_VBRIDGE:
        query_string = "(select unc_vtn_name, unc_vnode_name, controller_name, domain_id, "
            "ctrlr_vtn_name, ctrlr_vnode_name from "
            "(select unc_vtn_name, unc_vnode_name, controller_name, domain_id, "
            "ctrlr_vtn_name, ctrlr_vnode_name from im_vnode_rename_tbl where "
            "(unc_vtn_name, unc_vnode_name) IN (select vtn_name, vbridge_name "
            "from im_vbr_tbl) "
            "union all select vtn_name, vbridge_name, controller_name, domain_id, "
            "vtn_name, vbridge_name from im_vbr_tbl where flags & 3 = 0) as temp  where "
            "ctrlr_vtn_name = ? and ctrlr_vnode_name > ?) order by ctrlr_vtn_name, ctrlr_vnode_name ";
        break;
      case UNC_KT_VROUTER:
        query_string = "(select unc_vtn_name, unc_vnode_name, controller_name, domain_id, "
            "ctrlr_vtn_name, ctrlr_vnode_name from "
            "(select unc_vtn_name, unc_vnode_name, controller_name, domain_id, "
            "ctrlr_vtn_name, ctrlr_vnode_name from im_vnode_rename_tbl where "
            "(unc_vtn_name, unc_vnode_name) IN (select vtn_name, vrouter_name "
            "from im_vrt_tbl) "
            "union all select vtn_name, vrouter_name, controller_name, domain_id, "
            "vtn_name, vrouter_name from im_vrt_tbl where flags & 3 = 0) as temp  where "
            "ctrlr_vtn_name = ? and ctrlr_vnode_name > ?) order by ctrlr_vtn_name, ctrlr_vnode_name ";
        break;
      case UNC_KT_VLINK:
        query_string = "(select unc_vtn_name, unc_vlink_name, controller_name, domain_id, "
            "ctrlr_vtn_name, ctrlr_vlink_name from "
            "(select unc_vtn_name, unc_vlink_name, controller_name, domain_id, "
            "ctrlr_vtn_name, ctrlr_vlink_name from im_vlink_rename_tbl "
            "union all select vtn_name, vlink_name, controller1_name, domain1_id, "
            "vtn_name, vlink_name from im_vlink_tbl where key_flags & 3 = 0) as temp  where "
            "ctrlr_vtn_name = ? and ctrlr_vlink_name > ?) order by ctrlr_vtn_name, ctrlr_vlink_name ";
        break;
      default:
        break;
    }
  } else if (op == UNC_OP_READ_SIBLING_BEGIN) {
    switch (kt) {
      case UNC_KT_FLOWLIST:
        query_string = "(select unc_flowlist_name, ctrlr_name,"
                       "ctrlr_flowlist_name from im_flowlist_rename_tbl "
                        "union all select flowlist_name, ctrlr_name,"
                       "flowlist_name from im_flowlist_ctrlr_tbl "
                        "where flags & 1 = 0) order by ctrlr_flowlist_name ";
        break;
      case UNC_KT_POLICING_PROFILE:
        query_string = "(select unc_policingprofile_name, ctrlr_name,"
                     "ctrlr_policingprofile_name from im_policingprofile_rename_tbl "
                       " union all select policingprofile_name, ctrlr_name,"
                     "policingprofile_name from im_policingprofile_ctrlr_tbl "
                        "where flags & 1 = 0) order by ctrlr_policingprofile_name";
        break;
      case UNC_KT_VTN:
        query_string = "(select unc_vtn_name, controller_name, domain_id, "
                        "ctrlr_vtn_name from im_vtn_rename_tbl  union all "
                        "select vtn_name, controller_name, domain_id, vtn_name "
                        "from im_vtn_ctrlr_tbl  where flags & 1 = 0) "
                        "order by ctrlr_vtn_name ";
        break;
      case UNC_KT_VBRIDGE:
        query_string = "(select unc_vtn_name, unc_vnode_name, controller_name, domain_id, "
            "ctrlr_vtn_name, ctrlr_vnode_name from "
            "(select unc_vtn_name, unc_vnode_name, controller_name, domain_id, "
            "ctrlr_vtn_name, ctrlr_vnode_name from im_vnode_rename_tbl where "
            "(unc_vtn_name, unc_vnode_name) IN (select vtn_name, vbridge_name "
            "from im_vbr_tbl) "
            "union all select vtn_name, vbridge_name, controller_name, domain_id, "
            "vtn_name, vbridge_name from im_vbr_tbl where flags & 3 = 0) as temp  where "
            "ctrlr_vtn_name = ?) order by ctrlr_vtn_name, ctrlr_vnode_name ";
        break;
      case UNC_KT_VROUTER:
        query_string = "(select unc_vtn_name, unc_vnode_name, controller_name, domain_id, "
            "ctrlr_vtn_name, ctrlr_vnode_name from "
            "(select unc_vtn_name, unc_vnode_name, controller_name, domain_id, "
            "ctrlr_vtn_name, ctrlr_vnode_name from im_vnode_rename_tbl where "
            "(unc_vtn_name, unc_vnode_name) IN (select vtn_name, vrouter_name "
            "from im_vrt_tbl) "
            "union all select vtn_name, vrouter_name, controller_name, domain_id, "
            "vtn_name, vrouter_name from im_vrt_tbl where flags & 3 = 0) as temp  where "
            "ctrlr_vtn_name = ?) order by ctrlr_vtn_name, ctrlr_vnode_name ";
        break;
      case UNC_KT_VLINK:
        query_string = "(select unc_vtn_name, unc_vlink_name, controller_name, domain_id, "
            "ctrlr_vtn_name, ctrlr_vlink_name from "
            "(select unc_vtn_name, unc_vlink_name, controller_name, domain_id, "
            "ctrlr_vtn_name, ctrlr_vlink_name from im_vlink_rename_tbl "
            "union all select vtn_name, vlink_name, controller1_name, domain1_id, "
            "vtn_name, vlink_name from im_vlink_tbl where key_flags & 3 = 0) as temp  where "
            "ctrlr_vtn_name = ? ) order by ctrlr_vtn_name, ctrlr_vlink_name ";
        break;
      default:
        break;
    }
  }
  return query_string;
}

upll_rc_t MoMgrImpl::ReadImportDB(ConfigKeyVal *&in_key,
                                   IpcReqRespHeader *header,
                                   DalDmlIntf *dmi) {
  UPLL_FUNC_TRACE;
  uint8_t rename = 0;
  uint32_t count = 0;
  ConfigKeyVal *ikey = NULL;
  upll_rc_t result_code = UPLL_RC_SUCCESS;
  result_code = GetChildConfigKey(ikey, in_key);
  if (UPLL_RC_SUCCESS != result_code)
    return result_code;
  const uudst::kDalTableIndex tbl_index = GetTable(RENAMETBL,
                                                   header->datatype);
  void *tkey = (ikey)?ikey->get_key():NULL;
  if (!tkey) {
    delete ikey;
    return UPLL_RC_ERR_GENERIC;
  }
  if (tbl_index >= uudst::kDalNumTables) {
    UPLL_LOG_DEBUG(" Invalid Table index - %d", tbl_index);
    delete ikey;
    return UPLL_RC_ERR_GENERIC;
  }
  DalCursor *dal_cursor_handle = NULL;
  UPLL_LOG_TRACE("tbl_index is %d", tbl_index);
  unc_keytype_operation_t op = header->operation;

  if (!READ_OP(op)) {
    UPLL_LOG_INFO("Exiting MoMgrImpl::ReadConfigDB");
    delete ikey;
    return UPLL_RC_ERR_GENERIC;
  }
#if 0
  uint16_t max_record_count = 1;
  result_code = GetRenamedUncKey(ikey, header->datatype,
                                 dmi, NULL);
  if (UPLL_RC_SUCCESS != result_code &&
      UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code) {
    UPLL_LOG_DEBUG("GetRenamedUncKey is Failed %d", result_code);
    return result_code;
  }
#endif

  if (op == UNC_OP_READ_SIBLING_BEGIN || op == UNC_OP_READ_SIBLING) {
    DalBindInfo *dal_bind_info = new DalBindInfo(tbl_index);
    result_code = BindImportDB(ikey, dal_bind_info, header->datatype,
                               RENAMETBL);
    if (result_code != UPLL_RC_SUCCESS) {
      if (dal_bind_info) delete dal_bind_info;
      UPLL_LOG_INFO("Exiting MoMgrImpl::ReadConfigDB result code %d",
                    result_code);
      delete ikey;
      return result_code;
    }
    std::string query_string = GetReadImportQueryString(op,
                                                        ikey->get_key_type());
    if (query_string.empty()) {
      UPLL_LOG_TRACE("Null Query String for Operation(%d) KeyType(%d)",
                     op, ikey->get_key_type());
      if (dal_bind_info) delete dal_bind_info;
      delete ikey;
      return UPLL_RC_ERR_GENERIC;
    }
    result_code = DalToUpllResCode(
        dmi->ExecuteAppQueryMultipleRecords(query_string, header->rep_count,
                                            dal_bind_info, &dal_cursor_handle));
    ConfigKeyVal *end_resp = NULL;
    bool flag = false;
    while (result_code == UPLL_RC_SUCCESS && ((count < header->rep_count) ||
                                              (header->rep_count == 0))) {
      result_code = DalToUpllResCode(dmi->GetNextRecord(dal_cursor_handle));
      if (UPLL_RC_SUCCESS == result_code) {
        ConfigKeyVal *tkey = NULL;
        val_rename_vtn_t *val = (ikey)?reinterpret_cast<val_rename_vtn_t*>
            (GetVal(ikey)):NULL;
        if (val) {
          val->valid[UPLL_IDX_NEW_NAME_RVTN] = UNC_VF_VALID;
        }
        UPLL_LOG_TRACE("GetNextRecord %s", ikey->ToStrAll().c_str());
        result_code = DupConfigKeyVal(tkey, ikey, RENAMETBL);
        if (result_code != UPLL_RC_SUCCESS) {
          UPLL_LOG_DEBUG("Dup failed error %d", result_code);
          delete ikey;
          delete dal_bind_info;
          dmi->CloseCursor(dal_cursor_handle);
          return result_code;
        }
        flag = true;
        rename = 0;
        result_code = UpdateConfigDB(tkey, header->datatype, UNC_OP_READ,
                                     dmi, RENAMETBL);
        if (result_code == UPLL_RC_ERR_INSTANCE_EXISTS)
          rename = 1;
        else  if (UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code) {
          UPLL_LOG_DEBUG("ReadConfigDB failed %d", result_code);
          delete ikey;
          dmi->CloseCursor(dal_cursor_handle);
          delete dal_bind_info;
          DELETE_IF_NOT_NULL(tkey);
          return result_code;
        }
        result_code = SwapKey(tkey, rename);
        UPLL_LOG_TRACE("After No SwapKey %s", ikey->ToStrAll().c_str());
        ConfigKeyVal *prev_key = tkey;
        if (!end_resp)
          end_resp = tkey;
        else
          prev_key->AppendCfgKeyVal(tkey);
        count++;
        UPLL_LOG_TRACE("end_resp %s", end_resp->ToStrAll().c_str());
      }
    }
    header->rep_count = count;
    result_code = (UPLL_RC_ERR_NO_SUCH_INSTANCE == result_code && flag)?
        UPLL_RC_SUCCESS:result_code;
    if (dal_cursor_handle)
      dmi->CloseCursor(dal_cursor_handle);
    if (dal_bind_info)
      delete dal_bind_info;
    if (result_code == UPLL_RC_SUCCESS) {
      if (end_resp)
        ikey->ResetWith(end_resp);
      DELETE_IF_NOT_NULL(end_resp);
      in_key->ResetWith(ikey);
      DELETE_IF_NOT_NULL(ikey);
      UPLL_LOG_TRACE("ResetWith is Called");
    } else {
      delete ikey;
      return result_code;
    }
  } else if (op == UNC_OP_READ) {
#if 0  // tbl is set, but not used.
    MoMgrTables tbl = MAINTBL;
    if (UNC_KT_VTN == ikey->get_key_type() ||
        UNC_KT_FLOWLIST == ikey->get_key_type() ||
        UNC_KT_POLICING_PROFILE == ikey->get_key_type())
      tbl = CTRLRTBL;
#endif
    /* We are not allow to read using the UNC Name
    */
#if 1
    DbSubOp dbop = { kOpReadExist, kOpMatchNone, kOpInOutNone};

    ConfigKeyVal *temp = NULL;
    result_code = DupConfigKeyVal(temp, ikey, RENAMETBL);
    if (UPLL_RC_SUCCESS != result_code) {
      UPLL_LOG_DEBUG("DupConfigKeyVal Failed");
      DELETE_IF_NOT_NULL(ikey);
      return result_code;
    }
    result_code = UpdateConfigDB(temp, header->datatype, UNC_OP_READ,
                                 dmi, &dbop, RENAMETBL);
    if (UPLL_RC_ERR_INSTANCE_EXISTS != result_code &&
        UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code) {
      UPLL_LOG_DEBUG("ReadConfigDB failed %d", result_code);
      DELETE_IF_NOT_NULL(temp);
      return result_code;
    }
    if (UPLL_RC_ERR_INSTANCE_EXISTS == result_code) {
      UPLL_LOG_DEBUG("Read Not allowed by using UNC Name");
      delete ikey;
      DELETE_IF_NOT_NULL(temp);
      return UPLL_RC_ERR_NO_SUCH_INSTANCE;
    }
    DELETE_IF_NOT_NULL(temp);
#endif
    rename  = 0;
    result_code = GetRenamedUncKey(ikey, header->datatype,
                                   dmi, NULL);
    if (UPLL_RC_SUCCESS != result_code &&
        UPLL_RC_ERR_NO_SUCH_INSTANCE != result_code) {
      UPLL_LOG_DEBUG("GetRenamedUncKey is Failed %d", result_code);
      delete ikey;
      return result_code;
    }
    if (UPLL_RC_ERR_NO_SUCH_INSTANCE == result_code) {
      result_code = UpdateConfigDB(ikey, header->datatype, UNC_OP_READ,
                                   dmi, MAINTBL);
      if (result_code != UPLL_RC_ERR_INSTANCE_EXISTS) {
        UPLL_LOG_DEBUG("VTN doesn't exist in IMPORT DB. Error code : %d",
                       result_code);
        delete ikey;
        return result_code;
      } else
        result_code = UPLL_RC_SUCCESS;
      ikey->SetCfgVal(NULL);
    } else {
      DbSubOp dbop = {kOpReadSingle, kOpMatchNone, kOpInOutNone};
      result_code = ReadConfigDB(ikey, header->datatype, header->operation,
                                 dbop, dmi, RENAMETBL);
      if (UPLL_RC_SUCCESS != result_code) {
        UPLL_LOG_DEBUG("ReadConfigDB failed %d", result_code);
        delete ikey;
        return result_code;
      }
      rename = 1;
    }
    result_code = SwapKey(ikey, rename);
    UPLL_LOG_TRACE("After No SwapKey %s", ikey->ToStrAll().c_str());
    in_key->ResetWith(ikey);
    delete ikey;
  } else {
    UPLL_LOG_TRACE("Unexpected Operation : %d", op);
    delete ikey;
    return UPLL_RC_ERR_GENERIC;
  }
  return result_code;
}



















#if 0
template upll_rc_t
MoMgrImpl::GetUninitOperState<val_vlink_st_t, val_db_vlink_st_t>
(ConfigKeyVal *&ck_vn, DalDmlIntf   *dmi);
template upll_rc_t
MoMgrImpl::GetUninitOperState<val_vbr_st_t, val_db_vbr_st_t>
(ConfigKeyVal *&ck_vn, DalDmlIntf   *dmi);
template upll_rc_t
MoMgrImpl::GetUninitOperState<val_vbr_if_st_t, val_db_vbr_if_st_t>
(ConfigKeyVal *&ck_vn, DalDmlIntf *dmi);
template upll_rc_t
MoMgrImpl::GetUninitOperState<val_vrt_if_st_t, val_db_vrt_if_st_t>
(ConfigKeyVal *&ck_vn, DalDmlIntf *dmi);
template upll_rc_t
MoMgrImpl::GetUninitOperState<val_vrt_st_t, val_db_vrt_st_t>
(ConfigKeyVal *&ck_vn, DalDmlIntf *dmi);
template upll_rc_t
MoMgrImpl::GetUninitOperState<val_vtunnel_st_t, val_db_vtunnel_st_t>
(ConfigKeyVal *&ck_vn, DalDmlIntf *dmi);
template upll_rc_t
MoMgrImpl::GetUninitOperState<val_vtunnel_if_st_t, val_db_vtunnel_if_st_t>
(ConfigKeyVal *&ck_vn, DalDmlIntf *dmi);
template upll_rc_t
MoMgrImpl::GetUninitOperState<val_vtep_st_t, val_db_vtep_st_t>
(ConfigKeyVal *&ck_vn, DalDmlIntf *dmi);
template upll_rc_t
MoMgrImpl::GetUninitOperState<val_vtep_if_st_t, val_db_vtep_if_st_t>
(ConfigKeyVal *&ck_vn, DalDmlIntf   *dmi);
template upll_rc_t
MoMgrImpl::GetUninitOperState<val_vtn_st_t, val_db_vtn_st_t>
(ConfigKeyVal *&ck_vn, DalDmlIntf   *dmi);
#endif
#endif


}  // namespace kt_momgr
}  // namespace upll
}  // namespace unc
