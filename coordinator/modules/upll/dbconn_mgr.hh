/*
 * Copyright (c) 2013-2014 NEC Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#ifndef UPLL_DBCONN_MGR_HH_
#define UPLL_DBCONN_MGR_HH_

#include <list>

#include "cxx/pfcxx/synch.hh"

#include "unc/upll_errno.h"
#include "uncxx/upll_log.hh"

#include "dal/dal_odbc_mgr.hh"

namespace unc {
namespace upll {
namespace config_momgr {

using unc::upll::dal::DalOdbcMgr;
namespace uudal = unc::upll::dal;

class UpllDbConnMgr {
 public:
  explicit UpllDbConnMgr(size_t max_ro_conns) : ro_conn_sem_(max_ro_conns) {
    config_rw_conn_ = alarm_rw_conn_ = NULL;
    max_ro_conns_ = max_ro_conns;
    active_ro_conns_cnt_= 0;
  }
  upll_rc_t InitializeDbConnections();
  upll_rc_t TerminateAllDbConns();
  // GetConfigRwConn() should be called after InitializeDbConnections()
  // It cannot be called after TerminateAllDbConns()
  DalOdbcMgr *GetConfigRwConn();
  // GetAlarmRwConn() should be called after InitializeDbConnections()
  // It cannot be called after TerminateAllDbConns()
  DalOdbcMgr *GetAlarmRwConn();
  void ReleaseRwConn(DalOdbcMgr *dom);

  inline size_t get_ro_conn_limit() const { return max_ro_conns_; }
  upll_rc_t AcquireRoConn(DalOdbcMgr **);
  upll_rc_t ReleaseRoConn(DalOdbcMgr *);
  // void DestroyRoConns();
  upll_rc_t DalOpen(DalOdbcMgr *dom, bool read_write_conn);
  upll_rc_t DalTxClose(DalOdbcMgr *dom, bool commit);

  static upll_rc_t ConvertDalResultCode(uudal::DalResultCode drc);
  void ConvertConnInfoToStr() const;

 private:
  class DbConn {
   public:
    DbConn() { in_use_cnt = 0; close_on_finish = false; }
    DalOdbcMgr dom; 	// DAL object instance which manages the ODBC connection
    uint32_t in_use_cnt;		// If >0, connection is allocated
    bool close_on_finish; 	// If true, the connection is closed after
    // the connection is returned.
  };
  DbConn* config_rw_conn_;  // shared connection
  DbConn* alarm_rw_conn_;   // shared connection
  size_t max_ro_conns_;
  size_t active_ro_conns_cnt_;
  std::list<DbConn*> ro_conn_pool_;  // not shared connection
  // stale_rw_conn_pool_: rw connections that need to be closed and destroyed
  std::list<DbConn*> stale_rw_conn_pool_;
  pfc::core::Mutex conn_mutex_;
  pfc::core::Semaphore ro_conn_sem_;

  upll_rc_t TerminateDbConn(DbConn *);
  upll_rc_t TerminateAllRoConns_NoLock();
};

}  // namespace config_momgr
}  // namespace upll
}  // namespace unc


#endif  // UPLL_DBCONN_MGR_HH_

