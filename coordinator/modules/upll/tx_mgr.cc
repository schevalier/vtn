/*
 * Copyright (c) 2012-2014 NEC Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#include "pfc/debug.h"
#include "cxx/pfcxx/synch.hh"
#include "unc/component.h"
#include "alarm.hh"
#include "uncxx/upll_log.hh"
#include "ctrlr_mgr.hh"
#include "config_mgr.hh"

namespace unc {
namespace upll {
namespace config_momgr {

using unc::upll::dal::DalOdbcMgr;
using namespace unc::upll::upll_util;

#define CALL_MOMGRS_PREORDER(func, ...)                                     \
{                                                                         \
  const std::list<unc_key_type_t> *lst = cktt_.get_preorder_list();       \
  for (std::list<unc_key_type_t>::const_iterator it = lst->begin();       \
       it != lst->end(); it++) {                                          \
    const unc_key_type_t kt(*it);                                         \
    std::map<unc_key_type_t, MoManager*>::iterator momgr_it =		\
    upll_kt_momgrs_.find(kt);					\
    if (momgr_it != upll_kt_momgrs_.end()) {				\
      UPLL_LOG_DEBUG("KT: %u; kt_name: %s", kt, kt_name_map_[kt].c_str());\
      MoManager *momgr = momgr_it->second;				\
      urc = momgr->func(kt, __VA_ARGS__);                                 \
      if (urc != UPLL_RC_SUCCESS) {                                       \
        UPLL_LOG_WARN("Error = %d, KT: %s", urc, kt_name_map_[kt].c_str());\
        break;                                                            \
      }                                                                   \
      if ((urc = ContinueActiveProcess()) != UPLL_RC_SUCCESS) {           \
        UPLL_LOG_WARN("Error = %d, KT: %s", urc, kt_name_map_[kt].c_str());\
        break;                                                            \
      }                                                                   \
    }                                                                     \
  }                                                                       \
}

#define CALL_MOMGRS_REVERSE_ORDER(func, ...)                                \
{                                                                         \
  const std::list<unc_key_type_t> *lst = cktt_.get_reverse_postorder_list();\
  for (std::list<unc_key_type_t>::const_iterator it = lst->begin();       \
       it != lst->end(); it++) {                                          \
    const unc_key_type_t kt(*it);                                         \
    std::map<unc_key_type_t, MoManager*>::iterator momgr_it =		\
    upll_kt_momgrs_.find(kt);					\
    if (momgr_it != upll_kt_momgrs_.end()) {				\
      UPLL_LOG_DEBUG("KT: %u; kt_name: %s", kt, kt_name_map_[kt].c_str());\
      MoManager *momgr = momgr_it->second;				\
      urc = momgr->func(kt, __VA_ARGS__);                                 \
      if (urc != UPLL_RC_SUCCESS) {                                       \
        UPLL_LOG_WARN("Error = %d, KT: %s", urc, kt_name_map_[kt].c_str());\
        break;                                                            \
      }                                                                   \
      if ((urc = ContinueActiveProcess()) != UPLL_RC_SUCCESS) {           \
        UPLL_LOG_WARN("Error = %d, KT: %s", urc, kt_name_map_[kt].c_str());\
        break;                                                            \
      }                                                                   \
    }                                                                     \
  }                                                                       \
}

upll_rc_t UpllConfigMgr::ValidateCommit(const char *caller) {
  UPLL_FUNC_TRACE;
  if (IsShuttingDown()) {
    UPLL_LOG_WARN("%s: Shutting down, cannot commit", caller);
    return UPLL_RC_ERR_SHUTTING_DOWN;
  }

  if (!IsActiveNode()) {
    UPLL_LOG_WARN("%s: Node is not active, cannot commit", caller);
    return UPLL_RC_ERR_NOT_SUPPORTED_BY_STANDBY;
  }

  pfc::core::ScopedMutex lock(commit_mutex_lock_);
  if (commit_in_progress_ == false) {
    UPLL_LOG_WARN("%s: Commit is not in progress", caller);
    return UPLL_RC_ERR_NOT_ALLOWED_AT_THIS_TIME;
  }

  return UPLL_RC_SUCCESS;
}

upll_rc_t UpllConfigMgr::ValidateAudit(const char *caller,
                                       const char *req_ctrlr_id) {
  UPLL_FUNC_TRACE;
  if (IsShuttingDown()) {
    UPLL_LOG_WARN("%s: Shutting down, cannot audit", caller);
    return UPLL_RC_ERR_SHUTTING_DOWN;
  }

  if (!IsActiveNode()) {
    UPLL_LOG_WARN("%s: Node is not active, cannot audit", caller);
    return UPLL_RC_ERR_NOT_SUPPORTED_BY_STANDBY;
  }

  pfc::core::ScopedMutex lock(audit_mutex_lock_);
  if (audit_in_progress_ == false) {
    UPLL_LOG_WARN("%s: Audit is not in progress", caller);
    return UPLL_RC_ERR_NOT_ALLOWED_AT_THIS_TIME;
  }
  if (0 != audit_ctrlr_id_.compare(req_ctrlr_id)) {
    UPLL_LOG_WARN("%s: Requested controller: %s, audit_ctrlr_id_: %s",
                  caller, req_ctrlr_id, audit_ctrlr_id_.c_str());
    return UPLL_RC_ERR_NOT_ALLOWED_AT_THIS_TIME;
  }

  return UPLL_RC_SUCCESS;
}

upll_rc_t UpllConfigMgr::OnTxStart(uint32_t session_id, uint32_t config_id,
                                   ConfigKeyVal **err_ckv) {
  UPLL_FUNC_TRACE;
  upll_rc_t urc = UPLL_RC_ERR_GENERIC;

  commit_mutex_lock_.lock();
  if (commit_in_progress_ == true) {
    UPLL_LOG_WARN("Another commit is in progress");
    return UPLL_RC_ERR_NOT_ALLOWED_AT_THIS_TIME;
  }
  commit_in_progress_ = true;
  commit_mutex_lock_.unlock();

  if (UPLL_RC_SUCCESS != (urc = ValidateCommit(__FUNCTION__))) {
    return urc;
  }

  affected_ctrlr_set_.clear();

  ScopedConfigLock scfg_lock(cfg_lock_,
                             UPLL_DT_CANDIDATE, ConfigLock::CFG_WRITE_LOCK,
                             UPLL_DT_RUNNING, ConfigLock::CFG_READ_LOCK);

  DalOdbcMgr *dbinst = dbcm_->GetConfigRwConn();

  CALL_MOMGRS_REVERSE_ORDER(TxUpdateController, session_id, config_id,
                            kUpllUcpDelete, &affected_ctrlr_set_, dbinst,
                            err_ckv);

  if (urc != UPLL_RC_SUCCESS) {
    dbcm_->DalTxClose(dbinst, false);
    dbcm_->ReleaseRwConn(dbinst);
    return urc;
  }

  CALL_MOMGRS_PREORDER(TxUpdateController, session_id, config_id,
                       kUpllUcpCreate, &affected_ctrlr_set_, dbinst, err_ckv);

  if (urc != UPLL_RC_SUCCESS) {
    dbcm_->DalTxClose(dbinst, false);
    dbcm_->ReleaseRwConn(dbinst);
    return urc;
  }

  CALL_MOMGRS_PREORDER(TxUpdateController, session_id, config_id,
                       kUpllUcpUpdate, &affected_ctrlr_set_, dbinst, err_ckv);

  if (urc != UPLL_RC_SUCCESS) {
    dbcm_->DalTxClose(dbinst, false);
    dbcm_->ReleaseRwConn(dbinst);
    return urc;
  }

  unc_key_type_t phase2_kts[] = { UNC_KT_POLICING_PROFILE_ENTRY,
    UNC_KT_POLICING_PROFILE,
    UNC_KT_FLOWLIST_ENTRY,
    UNC_KT_FLOWLIST
  };
  for (unsigned int i = 0; i < sizeof(phase2_kts)/sizeof(phase2_kts[0]); i++) {
    const unc_key_type_t phase2_kt(phase2_kts[i]);
    std::map<unc_key_type_t, MoManager*>::iterator momgr_it =
        upll_kt_momgrs_.find(phase2_kt);
    if (momgr_it != upll_kt_momgrs_.end()) {
      UPLL_LOG_DEBUG("KT: %u; kt_name: %s",
                     phase2_kt, kt_name_map_[phase2_kt].c_str());
      MoManager *momgr = momgr_it->second;
      if (momgr == NULL)
        continue;
      urc = momgr->TxUpdateController(phase2_kt,
                                      session_id,
                                      config_id,
                                      kUpllUcpDelete2,
                                      &affected_ctrlr_set_,
                                      dbinst,
                                      err_ckv);
      if (urc == UPLL_RC_SUCCESS) {
        urc = ContinueActiveProcess();
      }
      if (urc != UPLL_RC_SUCCESS) {
        UPLL_LOG_WARN("Error = %d, KT: %s",
                      urc, kt_name_map_[phase2_kt].c_str());
        break;
      }
    }
  }

  upll_rc_t db_urc = dbcm_->DalTxClose(dbinst, (urc == UPLL_RC_SUCCESS));
  dbcm_->ReleaseRwConn(dbinst);
  if (urc == UPLL_RC_SUCCESS) {
    urc = db_urc;
  }

  return urc;
}

upll_rc_t UpllConfigMgr::OnAuditTxStart(const char *ctrlr_id,
                                        uint32_t session_id,
                                        uint32_t config_id,
                                        ConfigKeyVal **err_ckv) {
  UPLL_FUNC_TRACE;
  upll_rc_t urc = UPLL_RC_ERR_GENERIC;

  if (UPLL_RC_SUCCESS != (urc = ValidateAudit(__FUNCTION__, ctrlr_id))) {
    return urc;
  }

  ScopedConfigLock scfg_lock(cfg_lock_,
                             UPLL_DT_AUDIT, ConfigLock::CFG_READ_LOCK,
                             UPLL_DT_RUNNING, ConfigLock::CFG_READ_LOCK);

  DalOdbcMgr *dbinst = dbcm_->GetConfigRwConn();

  affected_ctrlr_set_.clear();

  KTxCtrlrAffectedState ctrlr_affected = kCtrlrAffectedNoDiff;
  audit_ctrlr_affected_state_ = ctrlr_affected;
  CALL_MOMGRS_REVERSE_ORDER(AuditUpdateController, ctrlr_id, session_id,
                            config_id, kUpllUcpDelete, dbinst,
                            err_ckv, &ctrlr_affected);
  if (urc != UPLL_RC_SUCCESS) {
    dbcm_->DalTxClose(dbinst, false);
    dbcm_->ReleaseRwConn(dbinst);
    return urc;
  }

  CALL_MOMGRS_PREORDER(AuditUpdateController, ctrlr_id, session_id, config_id,
                       kUpllUcpCreate, dbinst, err_ckv,
                       &ctrlr_affected);
  if (urc != UPLL_RC_SUCCESS) {
    dbcm_->DalTxClose(dbinst, false);
    dbcm_->ReleaseRwConn(dbinst);
    return urc;
  }

  CALL_MOMGRS_PREORDER(AuditUpdateController, ctrlr_id, session_id, config_id,
                       kUpllUcpUpdate, dbinst, err_ckv, &ctrlr_affected);

  if (urc != UPLL_RC_SUCCESS) {
    dbcm_->DalTxClose(dbinst, false);
    dbcm_->ReleaseRwConn(dbinst);
    return urc;
  }

  unc_key_type_t phase2_kts[] = { UNC_KT_POLICING_PROFILE_ENTRY,
    UNC_KT_POLICING_PROFILE,
    UNC_KT_FLOWLIST_ENTRY,
    UNC_KT_FLOWLIST
  };
  for (unsigned int i = 0; i < sizeof(phase2_kts)/sizeof(phase2_kts[0]); i++) {
    const unc_key_type_t phase2_kt(phase2_kts[i]);
    std::map<unc_key_type_t, MoManager*>::iterator momgr_it =
        upll_kt_momgrs_.find(phase2_kt);
    if (momgr_it != upll_kt_momgrs_.end()) {
      UPLL_LOG_DEBUG("KT: %u; kt_name: %s", phase2_kt,
                     kt_name_map_[phase2_kt].c_str());
      MoManager *momgr = momgr_it->second;
      if (momgr == NULL)
        continue;
      urc = momgr->AuditUpdateController(phase2_kt, ctrlr_id, session_id,
                                         config_id, kUpllUcpDelete2,
                                         dbinst, err_ckv, &ctrlr_affected);
      if (urc == UPLL_RC_SUCCESS) {
        urc = ContinueActiveProcess();
      }
      if (urc != UPLL_RC_SUCCESS) {
        UPLL_LOG_WARN("Error = %d, KT: %s", urc,
                      kt_name_map_[phase2_kt].c_str());
        break;
      }
    }
  }

  upll_rc_t db_urc = dbcm_->DalTxClose(dbinst, (urc == UPLL_RC_SUCCESS));
  dbcm_->ReleaseRwConn(dbinst);
  if (urc == UPLL_RC_SUCCESS) {
    urc = db_urc;
  }

  if (urc == UPLL_RC_SUCCESS) {
    audit_ctrlr_affected_state_ = ctrlr_affected;
    if (ctrlr_affected == kCtrlrAffectedConfigDiff) {
      audit_mutex_lock_.lock();
      affected_ctrlr_set_.insert(audit_ctrlr_id_);
      audit_mutex_lock_.unlock();
    } else if (ctrlr_affected == kCtrlrAffectedOnlyCSDiff) {
      UPLL_LOG_INFO("Audit - only cs diff");
    } else {
      UPLL_LOG_INFO("No audit diff");
    }
  }

  return urc;
}

upll_rc_t UpllConfigMgr::OnTxVote(
    const std::set<std::string> **affected_ctrlr_set, ConfigKeyVal **err_ckv) {
  UPLL_FUNC_TRACE;
  upll_rc_t urc = UPLL_RC_ERR_GENERIC;

  if (UPLL_RC_SUCCESS != (urc = ValidateCommit(__FUNCTION__))) {
    return urc;
  }

  ScopedConfigLock scfg_lock(cfg_lock_,
                             UPLL_DT_CANDIDATE, ConfigLock::CFG_READ_LOCK,
                             UPLL_DT_RUNNING, ConfigLock::CFG_READ_LOCK);

  DalOdbcMgr *dbinst = dbcm_->GetConfigRwConn();

  CALL_MOMGRS_PREORDER(TxVote, dbinst, err_ckv);

  upll_rc_t db_urc = dbcm_->DalTxClose(dbinst, (urc == UPLL_RC_SUCCESS));
  dbcm_->ReleaseRwConn(dbinst);
  if (urc == UPLL_RC_SUCCESS) {
    urc = db_urc;
  }

  if (urc == UPLL_RC_SUCCESS)
    *affected_ctrlr_set = &affected_ctrlr_set_;

  return urc;
}

upll_rc_t UpllConfigMgr::OnAuditTxVote(
    const char *ctrlr_id, const std::set<std::string> **affected_ctrlr_set) {
  UPLL_FUNC_TRACE;
  upll_rc_t urc;
  if (UPLL_RC_SUCCESS != (urc = ValidateAudit(__FUNCTION__, ctrlr_id))) {
    return urc;
  }
  *affected_ctrlr_set = &affected_ctrlr_set_;

  return UPLL_RC_SUCCESS;
}

upll_rc_t UpllConfigMgr::OnTxVoteCtrlrStatus(
    list<CtrlrVoteStatus*> *ctrlr_vote_status) {
  UPLL_FUNC_TRACE;
  upll_rc_t urc = UPLL_RC_ERR_GENERIC;

  if (UPLL_RC_SUCCESS != (urc = ValidateCommit(__FUNCTION__))) {
    return urc;
  }

  ScopedConfigLock scfg_lock(cfg_lock_,
                             UPLL_DT_CANDIDATE, ConfigLock::CFG_READ_LOCK,
                             UPLL_DT_RUNNING, ConfigLock::CFG_READ_LOCK);

  DalOdbcMgr *dbinst = dbcm_->GetConfigRwConn();

  CALL_MOMGRS_PREORDER(TxVoteCtrlrStatus, ctrlr_vote_status, dbinst);

  upll_rc_t db_urc = dbcm_->DalTxClose(dbinst, (urc == UPLL_RC_SUCCESS));
  dbcm_->ReleaseRwConn(dbinst);
  if (urc == UPLL_RC_SUCCESS) {
    urc = db_urc;
  }

  return urc;
}

upll_rc_t UpllConfigMgr::OnAuditTxVoteCtrlrStatus(
    CtrlrVoteStatus *ctrlr_vote_status) {
  UPLL_FUNC_TRACE;
  upll_rc_t urc = UPLL_RC_ERR_GENERIC;

  if (UPLL_RC_SUCCESS != (urc = ValidateAudit(
              __FUNCTION__, ctrlr_vote_status->ctrlr_id.c_str()))) {
    return urc;
  }

  ScopedConfigLock scfg_lock(cfg_lock_,
                             UPLL_DT_AUDIT, ConfigLock::CFG_READ_LOCK,
                             UPLL_DT_RUNNING, ConfigLock::CFG_WRITE_LOCK);

  DalOdbcMgr *dbinst = dbcm_->GetConfigRwConn();

  CALL_MOMGRS_PREORDER(AuditVoteCtrlrStatus, ctrlr_vote_status, dbinst);

  upll_rc_t db_urc;
  if (UPLL_RC_SUCCESS != (db_urc = dbcm_->DalTxClose(
              dbinst, (urc == UPLL_RC_SUCCESS)))) {
    dbcm_->ReleaseRwConn(dbinst);
    return ((urc != UPLL_RC_SUCCESS) ? urc : db_urc);
  }
  dbcm_->ReleaseRwConn(dbinst);

  // Send an alarm
  if (ctrlr_vote_status->upll_ctrlr_result != UPLL_RC_SUCCESS &&
      ctrlr_vote_status->upll_ctrlr_result != UPLL_RC_ERR_CTR_DISCONNECTED) {
    upll_rc_t ctrmgr_urc;
    bool config_invalid = false;
    if ((ctrmgr_urc = (CtrlrMgr::GetInstance()->IsConfigInvalid(
                    ctrlr_vote_status->ctrlr_id.c_str(),
                    &config_invalid))) != UPLL_RC_SUCCESS) {
      UPLL_LOG_WARN("Error in IsConfigInvalid(%s). Urc=%d",
                    ctrlr_vote_status->ctrlr_id.c_str(), ctrmgr_urc);
      return ctrmgr_urc;
    }
    if (config_invalid != true) {
      CtrlrMgr::GetInstance()->UpdateInvalidConfig(
          ctrlr_vote_status->ctrlr_id.c_str(), true);
      SendInvalidConfigAlarm(ctrlr_vote_status->ctrlr_id, true);
    } else {
      UPLL_LOG_WARN("More than one time audit failed for controller %s",
                    ctrlr_vote_status->ctrlr_id.c_str());
    }
  }

  return urc;
}

upll_rc_t UpllConfigMgr::OnTxGlobalCommit(
    const std::set<std::string> **affected_ctrlr_set) {
  UPLL_FUNC_TRACE;
  upll_rc_t urc;
  if (UPLL_RC_SUCCESS != (urc = ValidateCommit(__FUNCTION__))) {
    UPLL_LOG_FATAL("TxGlobalCommit failed. Urc=%d", urc);
    return urc;
  }

  *affected_ctrlr_set = &affected_ctrlr_set_;
  return UPLL_RC_SUCCESS;
}

upll_rc_t UpllConfigMgr::OnAuditTxGlobalCommit(
    const char *ctrlr_id, const std::set<std::string> **affected_ctrlr_set) {
  UPLL_FUNC_TRACE;
  upll_rc_t urc;
  if (UPLL_RC_SUCCESS != (urc = ValidateAudit(__FUNCTION__, ctrlr_id))) {
    UPLL_LOG_FATAL("AuditTxGlobalCommit failed. Urc=%d", urc);
    return urc;
  }

  *affected_ctrlr_set = &affected_ctrlr_set_;

  return UPLL_RC_SUCCESS;
}

upll_rc_t UpllConfigMgr::OnTxCommitCtrlrStatus(
    uint32_t session_id, uint32_t config_id,
    list<CtrlrCommitStatus*> *ctrlr_commit_status) {
  UPLL_FUNC_TRACE;
  upll_rc_t urc = UPLL_RC_ERR_GENERIC;

  using unc::upll::ipc_util::ConfigNotifier;

  // During normal transaction ctrlr_commit_status will not be NULL.
  // For TcLibInterface::HandleAuditConfig, if TcServiceType is
  // TC_OP_CANDIDATE_COMMIT, then ctrlr_commit_status is NULL
  if (ctrlr_commit_status != NULL) {
    if (UPLL_RC_SUCCESS != (urc = ValidateCommit(__FUNCTION__))) {
      return urc;
    }
  }

  ScopedConfigLock scfg_lock(cfg_lock_,
                             UPLL_DT_CANDIDATE, ConfigLock::CFG_WRITE_LOCK,
                             UPLL_DT_RUNNING, ConfigLock::CFG_WRITE_LOCK);
  // TODO(a): Why do we need write lock on CANDIDATE?

  DalOdbcMgr *dbinst = dbcm_->GetConfigRwConn();

  CALL_MOMGRS_PREORDER(TxCopyCandidateToRunning, ctrlr_commit_status, dbinst);

  if (urc == UPLL_RC_SUCCESS) {
    unc_key_type_t state_kts[] = { UNC_KT_VLINK, UNC_KT_VBR_IF,
      UNC_KT_VRT_IF, UNC_KT_VBRIDGE,
      UNC_KT_VROUTER, UNC_KT_VTN };
    for (unsigned int i = 0; i < sizeof(state_kts)/sizeof(state_kts[0]); i++) {
      MoManager *momgr = GetMoManager(state_kts[i]);
      if (momgr == NULL)
        continue;
      urc = momgr->TxUpdateDtState(state_kts[i], session_id, config_id,
                                   dbinst);
      if (urc != UPLL_RC_SUCCESS) {
        UPLL_LOG_DEBUG("Failed to update status for KT %d", state_kts[i]);
        break;
      }
      if ((urc = ContinueActiveProcess()) != UPLL_RC_SUCCESS) {
        UPLL_LOG_WARN("Error = %d, TxUpdateDtState KT: %d", urc, state_kts[i]);
        break;
      }
    }
  }

  upll_rc_t db_urc = dbcm_->DalTxClose(dbinst, (urc == UPLL_RC_SUCCESS));
  dbcm_->ReleaseRwConn(dbinst);
  if (urc == UPLL_RC_SUCCESS) {
    urc = db_urc;
  }

  if (urc == UPLL_RC_SUCCESS) {
    candidate_dirty_qc_lock_.lock();
    candidate_dirty_qc_ = false;
    // Clearing the dirty flags(used for skipping DB Diff operation) if stored
    dbinst->ClearDirty();
    candidate_dirty_qc_lock_.unlock();
    // Send Config Notifications
    ConfigNotifier::SendBufferedNotificationsToUpllUser();
  } else {
    ConfigNotifier::CancelBufferedNotificationsToUpllUser();
  }

  if (urc != UPLL_RC_SUCCESS) {
    UPLL_LOG_FATAL("Committing candidate to running failed. Urc=%d", urc);
  }
  return urc;
}

upll_rc_t UpllConfigMgr::OnAuditTxCommitCtrlrStatus(
    CtrlrCommitStatus * ctrlr_commit_status) {
  UPLL_FUNC_TRACE;
  upll_rc_t urc = UPLL_RC_ERR_GENERIC;

  if (UPLL_RC_SUCCESS != (urc = ValidateAudit(
              __FUNCTION__, ctrlr_commit_status->ctrlr_id.c_str()))) {
    return urc;
  }

  ScopedConfigLock scfg_lock(cfg_lock_,
                             UPLL_DT_AUDIT, ConfigLock::CFG_READ_LOCK,
                             UPLL_DT_RUNNING, ConfigLock::CFG_WRITE_LOCK);

  DalOdbcMgr *dbinst = dbcm_->GetConfigRwConn();

  CALL_MOMGRS_PREORDER(AuditCommitCtrlrStatus, ctrlr_commit_status, dbinst);

  upll_rc_t db_urc;
  if (UPLL_RC_SUCCESS != (db_urc = dbcm_->DalTxClose(
              dbinst, (urc == UPLL_RC_SUCCESS)))) {
    dbcm_->ReleaseRwConn(dbinst);
    return ((urc != UPLL_RC_SUCCESS) ? urc : db_urc);
  }
  dbcm_->ReleaseRwConn(dbinst);

  bool assert_alarm = (ctrlr_commit_status->upll_ctrlr_result ==
                       UPLL_RC_SUCCESS) ? false : true;
  bool alarm_exists = false;
  upll_rc_t ctrmgr_urc;
  if ((ctrmgr_urc = (CtrlrMgr::GetInstance()->IsConfigInvalid(
                  ctrlr_commit_status->ctrlr_id.c_str(),
                  &alarm_exists))) != UPLL_RC_SUCCESS) {
    UPLL_LOG_INFO("Error in IsConfigInvalid(%s). Urc=%d",
                  ctrlr_commit_status->ctrlr_id.c_str(), ctrmgr_urc);
  } else {
    if (assert_alarm) {
      UPLL_LOG_ERROR("Audit failed for %s",
                     ctrlr_commit_status->ctrlr_id.c_str());
      if (!alarm_exists) {
        SendInvalidConfigAlarm(ctrlr_commit_status->ctrlr_id, assert_alarm);
        CtrlrMgr::GetInstance()->UpdateInvalidConfig(
            ctrlr_commit_status->ctrlr_id.c_str(), assert_alarm);
      }
    } else {
      UPLL_LOG_INFO("Audit successful for %s",
                    ctrlr_commit_status->ctrlr_id.c_str());
      if (alarm_exists) {
        SendInvalidConfigAlarm(ctrlr_commit_status->ctrlr_id, assert_alarm);
        CtrlrMgr::GetInstance()->UpdateInvalidConfig(
            ctrlr_commit_status->ctrlr_id.c_str(), assert_alarm);
      }
    }
  }

  if (urc != UPLL_RC_SUCCESS) {
    UPLL_LOG_FATAL("AuditCommitCtrlrStatus failed. Urc=%d", urc);
  }

  return urc;
}

upll_rc_t UpllConfigMgr::OnTxEnd() {
  UPLL_FUNC_TRACE;
  upll_rc_t urc = UPLL_RC_ERR_GENERIC;

  if (UPLL_RC_SUCCESS != (urc = ValidateCommit(__FUNCTION__))) {
    return urc;
  }

  ScopedConfigLock scfg_lock(cfg_lock_,
                             UPLL_DT_CANDIDATE, ConfigLock::CFG_WRITE_LOCK,
                             UPLL_DT_RUNNING, ConfigLock::CFG_WRITE_LOCK);

  DalOdbcMgr *dbinst = dbcm_->GetConfigRwConn();

  CALL_MOMGRS_PREORDER(TxEnd, dbinst);

  upll_rc_t db_urc = dbcm_->DalTxClose(dbinst, (urc == UPLL_RC_SUCCESS));
  dbcm_->ReleaseRwConn(dbinst);
  if (urc == UPLL_RC_SUCCESS) {
    urc = db_urc;
  }
  if (urc != UPLL_RC_SUCCESS) {
    UPLL_LOG_FATAL("TxEnd failed. Urc=%d", urc);
  }

  commit_mutex_lock_.lock();
  commit_in_progress_ = false;
  commit_mutex_lock_.unlock();

  affected_ctrlr_set_.clear();

  return urc;
}

upll_rc_t UpllConfigMgr::OnAuditTxEnd(const char *ctrlr_id) {
  UPLL_FUNC_TRACE;
  upll_rc_t urc;
  if (UPLL_RC_SUCCESS != (urc = ValidateAudit(__FUNCTION__, ctrlr_id))) {
    return urc;
  }

  // Nothing to do in DB

  return UPLL_RC_SUCCESS;
}

upll_rc_t UpllConfigMgr::OnAuditStart(const char *ctrlr_id) {
  UPLL_FUNC_TRACE;

  upll_rc_t urc = UPLL_RC_ERR_GENERIC;

  unc_keytype_ctrtype_t ctrlr_type;
  if (false == CtrlrMgr::GetInstance()->GetCtrlrType(
          ctrlr_id, UPLL_DT_RUNNING, &ctrlr_type)) {
    UPLL_LOG_INFO("Unknown controller. Cannot do audit for %s ",
                  ctrlr_id);
    return UPLL_RC_ERR_NO_SUCH_INSTANCE;
  }

  audit_mutex_lock_.lock();
  if (audit_in_progress_ == true) {
    audit_mutex_lock_.unlock();
    UPLL_LOG_WARN("Another audit is in progress for %s", ctrlr_id);
    return UPLL_RC_ERR_NOT_ALLOWED_AT_THIS_TIME;
  }
  audit_in_progress_ = true;
  audit_ctrlr_id_ = ctrlr_id;
  audit_mutex_lock_.unlock();

  if (UPLL_RC_SUCCESS != (urc = ValidateAudit(__FUNCTION__, ctrlr_id))) {
    return urc;
  }

  ScopedConfigLock scfg_lock(cfg_lock_,
                             UPLL_DT_AUDIT, ConfigLock::CFG_WRITE_LOCK,
                             UPLL_DT_RUNNING, ConfigLock::CFG_READ_LOCK);

  urc = ImportCtrlrConfig(ctrlr_id, UPLL_DT_AUDIT);

  return urc;
}

// UpllConfigMgr::OnAuditEnd() is called in two scenarios:
// 1. In normal audit operation at the end of Audit process either successful
// audit or failed audit. If updating to contorller failed, OnAuditEnd() is
// locall called.
// 2. During transition from Standby to Active.
upll_rc_t UpllConfigMgr::OnAuditEnd(const char *ctrlr_id, bool sby2act_trans) {
  UPLL_FUNC_TRACE;
  upll_rc_t urc = UPLL_RC_ERR_GENERIC;

  if (!sby2act_trans) {
    if (UPLL_RC_SUCCESS != (urc = ValidateAudit(__FUNCTION__, ctrlr_id))) {
      return urc;
    }
    // If only cs status is diffrent in the configuration then the
    // Call TxCommitCtrlrStatus to Update the config Status
    // During IMPORT->MERGE->COMMIT->AUDIT case, TC jumpt from
    // AuditTxStart to AuditEnd sice  there is no diff in the
    // configurations except cs-status in this case.
    //  In such case the configuration is not being sent to the controller
    //  during AUDIT operation
    if (audit_ctrlr_affected_state_ == kCtrlrAffectedOnlyCSDiff) {
      // Reset the audit controller affected state. Without reset,
      // stale value is present during failover
      audit_ctrlr_affected_state_ = kCtrlrAffectedNoDiff;
      CtrlrCommitStatus ccs(ctrlr_id, UPLL_RC_SUCCESS, UPLL_RC_SUCCESS);
      urc = OnAuditTxCommitCtrlrStatus(&ccs);
      if (urc != UPLL_RC_SUCCESS) {
        UPLL_LOG_FATAL("Failed to update config status in AuditEnd, Urc=%u",
                       urc);
        return urc;
      }
    }
  }
  // Reset the audit controller affected state. Without reset,
  // stale value is present during failover
  audit_ctrlr_affected_state_ = kCtrlrAffectedNoDiff;

  ScopedConfigLock scfg_lock(cfg_lock_,
                             UPLL_DT_AUDIT, ConfigLock::CFG_WRITE_LOCK);

  DalOdbcMgr *dbinst = dbcm_->GetConfigRwConn();

  CALL_MOMGRS_REVERSE_ORDER(AuditEnd, ctrlr_id, dbinst);

  if (urc != UPLL_RC_SUCCESS) {
    UPLL_LOG_FATAL("AuditEnd failed. Error:%d", urc);
  }

  upll_rc_t db_urc = dbcm_->DalTxClose(dbinst, (urc == UPLL_RC_SUCCESS));
  dbcm_->ReleaseRwConn(dbinst);
  if (urc == UPLL_RC_SUCCESS) {
    urc = db_urc;
  }

  if (urc != UPLL_RC_SUCCESS) {
    UPLL_LOG_FATAL("AuditEnd failed. Urc=%d", urc);
    return urc;
  }

  audit_mutex_lock_.lock();
  audit_in_progress_ = false;
  audit_ctrlr_id_ = "";
  audit_mutex_lock_.unlock();

  affected_ctrlr_set_.clear();

  return urc;
}

upll_rc_t UpllConfigMgr::OnLoadStartup() {
  UPLL_FUNC_TRACE;
  if (IsShuttingDown()) {
    UPLL_LOG_WARN("Shutting down, cannot load startup");
    return UPLL_RC_ERR_SHUTTING_DOWN;
  }

  if (!IsActiveNode()) {
    UPLL_LOG_WARN("Node is not active, cannot load startup");
    return UPLL_RC_ERR_NOT_SUPPORTED_BY_STANDBY;
  }

  upll_rc_t urc = UPLL_RC_ERR_GENERIC;
  ScopedConfigLock scfg_lock(cfg_lock_,
                             UPLL_DT_STARTUP, ConfigLock::CFG_READ_LOCK,
                             UPLL_DT_CANDIDATE, ConfigLock::CFG_WRITE_LOCK,
                             UPLL_DT_RUNNING, ConfigLock::CFG_WRITE_LOCK);
  ScopedConfigLock scfg_lck2(cfg_lock_,
                             UPLL_DT_IMPORT, ConfigLock::CFG_WRITE_LOCK,
                             UPLL_DT_AUDIT, ConfigLock::CFG_WRITE_LOCK);

  DalOdbcMgr *dbinst = dbcm_->GetConfigRwConn();

  CALL_MOMGRS_REVERSE_ORDER(ClearConfiguration, dbinst, UPLL_DT_CANDIDATE);
  if (urc != UPLL_RC_SUCCESS) {
    dbcm_->DalTxClose(dbinst, false);
    dbcm_->ReleaseRwConn(dbinst);
    UPLL_LOG_FATAL("Loading startup configuration failed. Urc=%d", urc);
    return urc;
  }

  CALL_MOMGRS_REVERSE_ORDER(ClearConfiguration, dbinst, UPLL_DT_RUNNING);
  if (urc != UPLL_RC_SUCCESS) {
    dbcm_->DalTxClose(dbinst, false);
    dbcm_->ReleaseRwConn(dbinst);
    UPLL_LOG_FATAL("Loading startup configuration failed. Urc=%d", urc);
    return urc;
  }

  CALL_MOMGRS_PREORDER(LoadStartup, dbinst);

  // Audit and Import tables have been already cleared as part of transitioning
  // to ACTIVE

  upll_rc_t db_urc = dbcm_->DalTxClose(dbinst, (urc == UPLL_RC_SUCCESS));
  dbcm_->ReleaseRwConn(dbinst);
  if (urc == UPLL_RC_SUCCESS) {
    urc = db_urc;
  }

  if (urc != UPLL_RC_SUCCESS) {
    UPLL_LOG_FATAL("Loading startup configuration failed. Urc=%d", urc);
  }

  if (urc == UPLL_RC_SUCCESS) {
    // SBY->ACT transition, the dirty flags would be lost, So making all the
    // tables dirty to perform ful DB Diff on commit/abort operation
    dbinst->MakeAllDirty();
  }
  return urc;
}

upll_rc_t UpllConfigMgr::OnSaveRunningConfig(uint32_t session_id) {
  UPLL_FUNC_TRACE;
  if (IsShuttingDown()) {
    UPLL_LOG_WARN("Shutting down, cannot save running");
    return UPLL_RC_ERR_SHUTTING_DOWN;
  }

  if (!IsActiveNode()) {
    UPLL_LOG_WARN("Node is not active, cannot save running");
    return UPLL_RC_ERR_NOT_SUPPORTED_BY_STANDBY;
  }

  upll_rc_t urc = UPLL_RC_ERR_GENERIC;
  ScopedConfigLock scfg_lock(cfg_lock_,
                             UPLL_DT_STARTUP, ConfigLock::CFG_WRITE_LOCK,
                             UPLL_DT_RUNNING, ConfigLock::CFG_READ_LOCK);

  DalOdbcMgr *dbinst = dbcm_->GetConfigRwConn();

  // Clearing StartUp configuration
  CALL_MOMGRS_REVERSE_ORDER(ClearConfiguration, dbinst, UPLL_DT_STARTUP);
  if (urc != UPLL_RC_SUCCESS) {
    dbcm_->DalTxClose(dbinst, false);
    dbcm_->ReleaseRwConn(dbinst);
    UPLL_LOG_FATAL("SaveRunningConfig failed. Urc=%d", urc);
    return urc;
  }

  // Copying Running configuration to Startup
  CALL_MOMGRS_PREORDER(CopyRunningToStartup, dbinst);

  upll_rc_t db_urc = dbcm_->DalTxClose(dbinst, (urc == UPLL_RC_SUCCESS));
  dbcm_->ReleaseRwConn(dbinst);
  if (urc == UPLL_RC_SUCCESS) {
    urc = db_urc;
  }

  if (urc != UPLL_RC_SUCCESS) {
    UPLL_LOG_FATAL("SaveRunningConfig failed. Urc=%d", urc);
  }

  return urc;
}

upll_rc_t UpllConfigMgr::OnAbortCandidateConfig(uint32_t session_id) {
  UPLL_FUNC_TRACE;

  if (IsShuttingDown()) {
    UPLL_LOG_WARN("Shutting down, cannot abort candidate");
    return UPLL_RC_ERR_SHUTTING_DOWN;
  }

  if (!IsActiveNode()) {
    UPLL_LOG_WARN("Node is not active, cannot abort candidate");
    return UPLL_RC_ERR_NOT_SUPPORTED_BY_STANDBY;
  }

  upll_rc_t urc = UPLL_RC_ERR_GENERIC;
  ScopedConfigLock scfg_lock(cfg_lock_,
                             UPLL_DT_CANDIDATE, ConfigLock::CFG_WRITE_LOCK,
                             UPLL_DT_RUNNING, ConfigLock::CFG_READ_LOCK);

  DalOdbcMgr *dbinst = dbcm_->GetConfigRwConn();
  // For TcLibInterface::HandleAuditConfig, session_id and config_id is 0
  // For such transactions, make all the tables dirty to perform full abort
  if (session_id == 0) {
    dbinst->MakeAllDirty();
  }

  CALL_MOMGRS_REVERSE_ORDER(CopyRunningToCandidate, dbinst, UNC_OP_DELETE);
  if (urc != UPLL_RC_SUCCESS) {
    dbcm_->DalTxClose(dbinst, false);
    dbcm_->ReleaseRwConn(dbinst);
    UPLL_LOG_FATAL("Abort candidate failed. Urc=%d", urc);
    return urc;
  }

  CALL_MOMGRS_PREORDER(CopyRunningToCandidate, dbinst, UNC_OP_CREATE);
  if (urc != UPLL_RC_SUCCESS) {
    dbcm_->DalTxClose(dbinst, false);
    dbcm_->ReleaseRwConn(dbinst);
    UPLL_LOG_FATAL("Abort candidate failed. Urc=%d", urc);
    return urc;
  }

  CALL_MOMGRS_PREORDER(CopyRunningToCandidate, dbinst, UNC_OP_UPDATE);

  upll_rc_t db_urc = dbcm_->DalTxClose(dbinst, (urc == UPLL_RC_SUCCESS));
  // Release DB Connection after ClearDirty
  if (urc == UPLL_RC_SUCCESS) {
    urc = db_urc;
  }

  if (urc != UPLL_RC_SUCCESS) {
    UPLL_LOG_FATAL("Abort candidate failed. Urc=%d", urc);
  }

  if (urc == UPLL_RC_SUCCESS) {
    candidate_dirty_qc_lock_.lock();
    candidate_dirty_qc_ = false;
    // Clearing the dirty flags(used for skipping DB Diff operation) if stored
    dbinst->ClearDirty();
    candidate_dirty_qc_lock_.unlock();
  }

  dbcm_->ReleaseRwConn(dbinst);
  return urc;
}

upll_rc_t UpllConfigMgr::OnClearStartupConfig(uint32_t session_id) {
  UPLL_FUNC_TRACE;
  if (IsShuttingDown()) {
    UPLL_LOG_WARN("Shutting down, cannot clear startup");
    return UPLL_RC_ERR_SHUTTING_DOWN;
  }

  if (!IsActiveNode()) {
    UPLL_LOG_WARN("Node is not active, cannot clear startup");
    return UPLL_RC_ERR_NOT_SUPPORTED_BY_STANDBY;
  }

  upll_rc_t urc = UPLL_RC_ERR_GENERIC;
  ScopedConfigLock scfg_lock(cfg_lock_,
                             UPLL_DT_STARTUP, ConfigLock::CFG_WRITE_LOCK);

  DalOdbcMgr *dbinst = dbcm_->GetConfigRwConn();

  CALL_MOMGRS_REVERSE_ORDER(ClearStartup, dbinst);

  upll_rc_t db_urc = dbcm_->DalTxClose(dbinst, (urc == UPLL_RC_SUCCESS));
  dbcm_->ReleaseRwConn(dbinst);
  if (urc == UPLL_RC_SUCCESS) {
    urc = db_urc;
  }

  if (urc != UPLL_RC_SUCCESS) {
    UPLL_LOG_FATAL("ClearStartupConfig failed. Urc=%d", urc);
  }

  return urc;
}

// Note: ClearImport is part of GlobalConfigService.  It is coded here to make
// use of CALL_MOMGRS_PREORDER
upll_rc_t UpllConfigMgr::ClearImport(uint32_t session_id, uint32_t config_id,
                                     bool sby2act_trans) {
  UPLL_FUNC_TRACE;
  upll_rc_t urc = UPLL_RC_ERR_GENERIC;

  pfc::core::ScopedMutex lock(import_mutex_lock_);

  if (!sby2act_trans) {
    if (import_in_progress_ == false) {
      UPLL_LOG_INFO("Import is not in progress. Clear cannot be done");
      return UPLL_RC_ERR_NOT_ALLOWED_AT_THIS_TIME;
    }
    if (UPLL_RC_SUCCESS != (urc = ValidateImport(
                session_id, config_id, import_ctrlr_id_.c_str(),
                UPLL_CLEAR_IMPORT_CONFIG_OP))) {
      return urc;
    }
  }

  ScopedConfigLock scfg_lock(cfg_lock_,
                             UPLL_DT_IMPORT, ConfigLock::CFG_WRITE_LOCK);

  DalOdbcMgr *dbinst = dbcm_->GetConfigRwConn();

  CALL_MOMGRS_REVERSE_ORDER(ImportClear, import_ctrlr_id_.c_str(), dbinst);

  upll_rc_t db_urc = dbcm_->DalTxClose(dbinst, (urc == UPLL_RC_SUCCESS));
  dbcm_->ReleaseRwConn(dbinst);
  if (urc == UPLL_RC_SUCCESS) {
    urc = db_urc;
  }

  if (urc != UPLL_RC_SUCCESS) {
    UPLL_LOG_FATAL("ClearImportConfig failed. Urc=%d", urc);
    return urc;
  }

  import_in_progress_ = false;
  import_ctrlr_id_ = "";

  return urc;
}

/**
 * @brief : This function sends invalid configuration alarm to node
 * manager
 */
bool UpllConfigMgr::SendInvalidConfigAlarm(string ctrlr_name,
                                           bool assert_alarm) {
  UPLL_FUNC_TRACE;
  const string vtn_name = "";
  std::string message;
  std::string message_summary;
  pfc::alarm::alarm_info_with_key_t data;

  if (assert_alarm) {
    message = "Invalid Configuration";
    message_summary = "Invalid Configuration";
    data.alarm_class = pfc::alarm::ALM_WARNING;
    data.alarm_kind = 1;   // assert alarm
  } else {
    message = "No invalid Configuration";
    message_summary = "No invalid Configuration";
    data.alarm_class = pfc::alarm::ALM_NOTICE;
    data.alarm_kind = 0;   // clear alarm
  }
  data.apl_No = UNCCID_LOGICAL;
  data.alarm_key_size = ctrlr_name.length();
  data.alarm_key = const_cast<uint8_t *>(
      reinterpret_cast<const uint8_t *>(ctrlr_name.c_str()));

  data.alarm_category = UPLL_INVALID_CONFIGURATION_ALARM;

  pfc::alarm::alarm_return_code_t ret = pfc::alarm::pfc_alarm_send_with_key(
      vtn_name, message, message_summary, &data, alarm_fd);
  if (ret != pfc::alarm::ALM_OK) {
    UPLL_LOG_INFO("Failed to %s invalid configuration alarm",
                  (assert_alarm) ? "assert" : "clear");
    return false;
  }

  return true;
}

}  // namespace config_momgr
}  // namespace upll
}  // namespace unc
