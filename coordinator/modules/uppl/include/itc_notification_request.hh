/*
 * Copyright (c) 2012-2014 NEC Corporation
 * All rights reserved.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */

/*
 * @brief	ITC Notification Request
 * @file        itc_notification_request.hh
 *
 */

#ifndef _ITC_NOTIFICATION_REQUEST_HH_
#define _ITC_NOTIFICATION_REQUEST_HH_

#include <pfcxx/module.hh>
#include <unc/keytype.h>
#include <string>
#include "physical_itc_req.hh"
#include "odbcm_connection.hh"
using pfc::core::ipc::ClientSession;
using unc::uppl::OdbcmConnectionHandler;

namespace unc {
namespace uppl {
class NotificationRequest : public ITCReq {
  public:
  /*NotificationRequest constructor*/
  NotificationRequest();
  /*NotificationRequest destructor*/
  ~NotificationRequest();
  /*This function invoke ProcessNotificationEvents to validate
   *Key_Type with event message
        and call corresponding Key_Type class function*/
  pfc_bool_t ProcessEvent(const IpcEvent &event);

  private:
  UncRespCode ProcessPortEvents(ClientSession *sess,
                                   uint32_t data_type,
                                   uint32_t operation);
  UncRespCode ProcessSwitchEvents(ClientSession *sess,
                                     uint32_t data_type,
                                     uint32_t operation);
  UncRespCode ProcessLinkEvents(ClientSession *sess,
                                   uint32_t data_type,
                                   uint32_t operation);
  UncRespCode ProcessControllerEvents(ClientSession *sess,
                                         uint32_t data_type,
                                         uint32_t operation);
  UncRespCode ProcessDomainEvents(ClientSession *sess,
                                     uint32_t data_type,
                                     uint32_t operation);
  UncRespCode ProcessLogicalPortEvents(ClientSession *sess,
                                          uint32_t data_type,
                                          uint32_t operation);
  UncRespCode ProcessLogicalMemeberPortEvents(
      ClientSession *sess,
      uint32_t data_type,
      uint32_t operation);
  void GetNotificationDT(OdbcmConnectionHandler *db_conn,
                         string controller_name,
                         uint32_t &data_type);
  /*This function process notification events*/
  UncRespCode ProcessNotificationEvents(const IpcEvent &event);
  /*This function process alarm events*/
  UncRespCode ProcessAlarmEvents(const IpcEvent &event);
  UncRespCode InvokeKtDriverEvent(
      OdbcmConnectionHandler *db_conn,
      uint32_t operation,
      uint32_t data_type,
      void *key_struct,
      void *new_val_struct,
      void *old_val_struct,
      uint32_t key_type);
};
}  // namespace uppl
}  // namespace unc

#endif  // _ITC_NOTIFICATION_REQUEST_HH_
