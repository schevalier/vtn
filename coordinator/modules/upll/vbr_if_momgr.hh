/*
 * Copyright (c) 2012-2013 NEC Corporation
 * All rights reserved.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#ifndef UNC_UPLL_VBR_IF_MOMGR_H
#define UNC_UPLL_VBR_IF_MOMGR_H

#include <string>
#include <sstream>
#include <set>
#include "momgr_impl.hh"
#include "vnode_child_momgr.hh"

namespace unc {
namespace upll {
namespace kt_momgr {

// TODO(owner):  remove once driver header file included
enum VexDefines {
  UPLL_IDX_VBR_IF_DRV_PM = 0,
  UPLL_IDX_VEXT_DRV_PM,
  UPLL_IDX_VEXT_IF_DRV_PM,
  UPLL_IDX_VEXT_LINK_DRV_PM
};

#if 0
enum vbr_if_numbers {
  VBR_IF_1 = 0x80,
  VBR_IF_2 = 0x40
};
#endif


#if 0
#define INTERFACE_TYPE 0xF0
#define INTERFACE_TYPE_BOUNDARY 0xC0
#define INTERFACE_TYPE_LINKED 0x30

enum if_type {
  kUnboundInterface = 0x0,
  kMappedInterface,
  kBoundaryInterface,
  kLinkedInterface 
};
#endif


/*TODO remove when including driver header file */
typedef struct val_drv_vbr_if {
  val_vbr_if_t            vbr_if_val;
  uint8_t                 vex_name[32];
  uint8_t                 vex_if_name[32];
  uint8_t                 vex_link_name[32];
  uint8_t                 valid[4];
} val_drv_vbr_if_t;



enum val_drv_vbr_if_index {
  UPLL_IDX_DESC_DRV_VBRI = 0,
  UPLL_IDX_ADMIN_STATUS_DRV_VBRI,
  UPLL_IDX_PM_DRV_VBRI
};

class VbrIfMoMgr : public VnodeChildMoMgr {
 private:
  static unc_key_type_t vbr_if_child[];
  static BindInfo       vbr_if_bind_info[];
  static BindInfo       key_vbr_if_maintbl_bind_info[];

  /* @brief      Returns portmap information if portmap is valid 
   *             Else returns NULL for portmap 
   *              
   * @param[in]   ikey     Pointer to ConfigKeyVal
   * @param[out]  valid_pm portmap is valid 
   * @param[out]  pm       pointer to portmap informtation if valid_pm
   *
   * @retval  UPLL_RC_SUCCESS      Completed successfully.
   * @retval  UPLL_RC_ERR_GENERIC  Generic failure.
   * 
   **/ 
   virtual upll_rc_t GetPortMap(ConfigKeyVal *ikey, uint8_t &valid_pm,
                                val_port_map_t *&pm) {
     UPLL_FUNC_TRACE;
     if (ikey == NULL) return UPLL_RC_ERR_GENERIC; 
     val_drv_vbr_if *drv_vbrif = reinterpret_cast<val_drv_vbr_if *>
                                                (GetVal(ikey));
     if (!drv_vbrif) {
       UPLL_LOG_DEBUG("Invalid param");
       return UPLL_RC_ERR_GENERIC;
     }
     val_vbr_if *ifval = &drv_vbrif->vbr_if_val;
     valid_pm = ifval->valid[UPLL_IDX_PM_VBRI];
     if (valid_pm == UNC_VF_VALID)
       pm = &ifval->portmap;
     else 
       pm = NULL;
     return UPLL_RC_SUCCESS;
   }

  /**
   * @brief  Gets the valid array position of the variable in the value 
   *         structure from the table in the specified configuration  
   *
   * @param[in]     val      pointer to the value structure 
   * @param[in]     indx     database index for the variable
   * @param[out]    valid    position of the variable in the valid array - 
   *                          NULL if valid does not exist.
   * @param[in]     dt_type  specifies the configuration
   * @param[in]     tbl      specifies the table containing the given value 
   *
   **/
   upll_rc_t GetValid(void *val, uint64_t indx, uint8_t *&valid,
      upll_keytype_datatype_t dt_type, MoMgrTables tbl ) {
    if (val == NULL) return UPLL_RC_ERR_GENERIC;
    if (tbl == MAINTBL) {
      switch (indx) {
        case uudst::vbridge_interface::kDbiOperStatus:
          valid = &(reinterpret_cast<val_vbr_if_st *>(val))->
                                  valid[UPLL_IDX_OPER_STATUS_VBRIS];
          break;
        case uudst::vbridge_interface::kDbiDownCount:
          valid = NULL;
          break;
        case uudst::vbridge_interface::kDbiAdminStatus:
          valid = &(reinterpret_cast<val_vbr_if *>(val))->
                                  valid[UPLL_IDX_ADMIN_STATUS_VBRI];
          break;
        case uudst::vbridge_interface::kDbiDesc:
          valid = &(reinterpret_cast<val_vbr_if *>(val))->
                                  valid[UPLL_IDX_DESC_VBRI];
          break;
        case uudst::vbridge_interface::kDbiValidPortMap:
          valid = &(reinterpret_cast<val_vbr_if *>(val))->
                                  valid[UPLL_IDX_PM_VBRI];
          break;
        case uudst::vbridge_interface::kDbiLogicalPortId:
          valid = &(reinterpret_cast<val_vbr_if *>(val))->portmap.
                                valid[UPLL_IDX_LOGICAL_PORT_ID_PM];
          break;
        case uudst::vbridge_interface::kDbiVlanId:
          valid = &(reinterpret_cast<val_vbr_if *>(val))->portmap.
                                valid[UPLL_IDX_VLAN_ID_PM];
          break;
        case uudst::vbridge_interface::kDbiTagged:
          valid = &(reinterpret_cast<val_vbr_if *>(val))->portmap.
                                valid[UPLL_IDX_TAGGED_PM];
          break;
        case uudst::vbridge_interface::kDbiVexName:
        case uudst::vbridge_interface::kDbiVexIfName:
        case uudst::vbridge_interface::kDbiVexLinkName:
          valid = &(reinterpret_cast<val_vbr_if *>(val))->portmap.
                                  valid[UPLL_IDX_LOGICAL_PORT_ID_PM];
          break;
        default:
          return UPLL_RC_ERR_GENERIC;
      }
    }
    return UPLL_RC_SUCCESS;
  }

  /**
   * @brief  Filters the attributes which need not be sent to controller
   *
   * @param[in/out]  val1   first record value instance.
   * @param[in]      val2   second record value instance.
   * @param[in]      audit  Not used for VTN
   * @param[in]      op     Operation to be performed
   *
   **/
  bool FilterAttributes(void *&val1, void *val2, bool audit_status,
                                unc_keytype_operation_t op);
  /**
   * @brief  Compares the valid value between two database records.
   * 	     if both the values are same, update the valid flag for 
   * 	     corresponding attribute as invalid in the first record. 
   *
   * @param[in/out]  val1   first record value instance.
   * @param[in]      val2   second record value instance.
   * @param[in]      audit  if true, CompareValidValue called from audit process.
   *
   **/
  bool CompareValidValue(void *&val1, void *val2, bool audit);

  upll_rc_t UpdateConfigStatus(ConfigKeyVal *req,
                               unc_keytype_operation_t op,
                               uint32_t driver_result,
                               ConfigKeyVal *upd_key,
                               DalDmlIntf   *dmi,
                               ConfigKeyVal *ctrlr_key = NULL);
  /**
   * @Brief  Validates the syntax of the specified key and value structure
   *         for KT_VBR_IF keytype
   *
   * @param[in]  req    This structure contains IpcReqRespHeader
   *                    (first 8 fields of input request structure).
   * @param[in]  ikey   ikey contains key and value structure.
   *
   * @retval  UPLL_RC_SUCCESS                Successful.
   * @retval  UPLL_RC_ERR_CFG_SYNTAX         Syntax error.
   * @retval  UPLL_RC_ERR_NO_SUCH_INSTANCE   key_vbr_if is not available.
   * @retval  UPLL_RC_ERR_GENERIC            Generic failure.
   * @retval  UPLL_RC_ERR_INVALID_OPTION1    option1 is not valid.
   * @retval  UPLL_RC_ERR_INVALID_OPTION2    option2 is not valid.
   */

  upll_rc_t ValidateMessage(IpcReqRespHeader *req, ConfigKeyVal *ikey);

   /**
     * @brief  Update config status for commit result and vote result.
     *
     * @param[in/out]  ckv_running  ConfigKeyVal instance.
     * @param[in]      cs_status    either UNC_CS_INVALID or UNC_CS_APPLIED.
     * @param[in]      phase        specify the phase (CREATE,DELETE or UPDATE)
     *
     **/
  upll_rc_t UpdateAuditConfigStatus(unc_keytype_configstatus_t cs_status,
                                     uuc::UpdateCtrlrPhase phase,
                                     ConfigKeyVal *&ckv_running);

  /**
   * @Brief  Checks if the specified key type(KT_VBR_IF) and
   *         associated attributes are supported on the given controller,
   *         based on the valid flag
   *
   * @param[in]  req              This structure contains IpcReqRespHeader
   *                              (first 8 fields of input request structure).
   * @param[in]  ikey             ikey contains key and value structure.
   * @param[in]  ctrlr_name       Controller id associated with ikey.
   *
   * @retval  UPLL_RC_SUCCESS        Validation succeeded.
   * @retval  UPLL_RC_ERR_GENERIC    Validation failure.
   * @retval  UPLL_RC_ERR_INVALID_OPTION1   Option1 is not valid.
   * @retval  UPLL_RC_ERR_INVALID_OPTION2   Option2 is not valid.
   */
  upll_rc_t ValidateCapability(IpcReqRespHeader *req,
               ConfigKeyVal *ikey, const char *ctrlr_name);

  /**
   * @Brief  Validates the syntax for KT_VBR_IF keytype value structure.
   *
   * @param[in]  val_vbr_if  KT_VBR_IF value structure.
   * @param[in]  operation  Operation type.
   * 
   * @retval  UPLL_RC_SUCCESS         validation succeeded.
   * @retval  UPLL_RC_ERR_CFG_SYNTAX  validation failed.
   */
  upll_rc_t  ValidateVbrIfValue(val_vbr_if *vbr_if_val,
                                unc_keytype_operation_t operation);

  /**
   * @Brief  Validates the syntax of the specified value structure
   *         for KT_VBR_IF keytype
   * @param[in]  val_vtn_neighbor  vtn neighbor value structure
   * @param[in]  operation   operation type.
   *
   * @retval  UPLL_RC_SUCCESS            Successful.
   * @retval  UPLL_RC_ERR_CFG_SYNTAX     Syntax error.
   *
   */
  upll_rc_t ValidateVtnNeighborValue(val_vtn_neighbor *vtn_neighbor,
      unc_keytype_operation_t operation);

  /**
   * @Brief  Checks if the specified key type and
   *         associated attributes are supported on the given controller,
   *         based on the valid flag.
   *
   * @param[in]  crtlr_name      Controller name.
   * @param[in]  ikey            Corresponding key and value structure.
   * @param[in]  operation       Operation name.
   *
   * @retval  UPLL_RC_SUCCESS                     validation succeeded.
   * @retval  UPLL_RC_ERR_EXCEEDS_RESOURCE_LIMIT  Instance count limit is exceeds.
   * @retval  UPLL_RC_ERR_NOT_SUPPORTED_BY_CTRLR  Attribute Not_Supported.
   * @retval  UPLL_RC_ERR_GENERIC                 Generic failure.
   */
  upll_rc_t ValVbrIfAttributeSupportCheck(const char *ctrlr_name,
      ConfigKeyVal *ikey, unc_keytype_operation_t operation,
      upll_keytype_datatype_t dt_type);

  /**
   * @Brief  Checks if the specified key type and
   *         associated attributes are supported on the given controller,
   *         based on the valid flag.
   *
   * @param[in]  crtlr_name      Controller name.
   * @param[in]  ikey            Corresponding key and value structure.
   *
   * @retval  UPLL_RC_SUCCESS                     validation succeeded.
   * @retval  UPLL_RC_ERR_EXCEEDS_RESOURCE_LIMIT  Instance count limit is exceeds.
   * @retval  UPLL_RC_ERR_NOT_SUPPORTED_BY_CTRLR  Attribute Not_Supported.
   * @retval  UPLL_RC_ERR_GENERIC                 Generic failure.
   */
  upll_rc_t ValVtnNeighborAttributeSupportCheck(const char *ctrlr_name,
      ConfigKeyVal *ikey);

   /**
   * @brief  Perform Semantic Check to check Different vbridges 
   *          contain same switch-id and vlan-id
   *
   * @param[in]      ikey        ConfigKeyVal
   * @param[in]      upll_rc_t   UPLL_RC_ERR_CFG_SEMANTIC on error
   *                                UPLL_RC_SUCCESS on success
   **/
    upll_rc_t ValidateAttribute(ConfigKeyVal *kval, 
                                DalDmlIntf *dmi,
                                IpcReqRespHeader *req = NULL);
  /**
     * @brief  Allocates for the specified val in the given configuration in the     * specified table.   
     *
     * @param[in]  ck_val   Reference pointer to configval structure allocated.      * @param[in]  dt_type  specifies the configuration candidate/running/state 
     * @param[in]  tbl      specifies if the corresponding table is the  main 
     *                      table / controller table or rename table.
     *
     * @retval     UPLL_RC_SUCCESS      Successfull completion.
     * @retval     UPLL_RC_ERR_GENERIC  Failure case.
     **/
  upll_rc_t AllocVal(ConfigVal *&ck_val, upll_keytype_datatype_t dt_type,
      MoMgrTables tbl = MAINTBL);
  /* Rename */
  bool GetRenameKeyBindInfo(unc_key_type_t key_type, BindInfo *&binfo,
       int &nattr, MoMgrTables tbl);
  upll_rc_t CopyToConfigKey(ConfigKeyVal *&okey,
                            ConfigKeyVal *ikey);

  /* @brief     To convert the value structure read from DB to 
   *            VTNService during READ operations
   * @param[in/out] ikey      Pointer to the ConfigKeyVal Structure             
   * 
   * @retval  UPLL_RC_SUCCESS                    Completed successfully.
   * @retval  UPLL_RC_ERR_GENERIC                Generic failure.
   *
   **/
  upll_rc_t AdaptValToVtnService(ConfigKeyVal *ikey);

 public:
  VbrIfMoMgr();
  virtual ~VbrIfMoMgr() {
    for (int i = 0; i < ntable; i++)
      if (table[i]) {
        delete table[i];
      }
    delete[] table;
  }

  /* @brief         Updates vbrif structure with vexternal information
   *                based on valid[PORTMAP] flag.
   *
   * @param[in/out] ikey     Pointer to the ConfigKeyVal Structure
   * @param[in]     datatype DB type.
   * @param[in]     dmi      Pointer to the DalDmlIntf(DB Interface)
   *
   * @retval  UPLL_RC_SUCCESS                    Completed successfully.
   * @retval  UPLL_RC_ERR_GENERIC                Generic failure.
   *
   **/
  upll_rc_t UpdateConfigVal(ConfigKeyVal *ikey,
                            upll_keytype_datatype_t datatype,
                            DalDmlIntf *dmi);


  /**
   * @brief      Method used to Update the Values in the specified key type.
   *
   * @param[in]  req                        contains first 8 fields of input request structure
   * @param[in]  ikey                       key and value structure
   * @param[in]  dmi                        Pointer to DalDmlIntf Class.
   *
   * @retval     UPLL_RC_SUCCESS            Successfull completion.
   * @retval     UPLL_RC_ERR_GENERIC        Failure case.
   */
  upll_rc_t UpdateMo(IpcReqRespHeader *req,
                             ConfigKeyVal *ikey,
                             DalDmlIntf *dmi);
  /**
    * @brief      Method to check if individual portions of a key are valid
    *
    * @param[in/out]  ikey                 pointer to ConfigKeyVal referring to a UNC resource
    * @param[in]      index                db index associated with the variable
    *
    * @retval         true                 input key is valid
    * @retval         false                input key is invalid.
    **/
  bool IsValidKey(void *tkey, uint64_t index);

  upll_rc_t GetVexternal(ConfigKeyVal *ikey, upll_keytype_datatype_t dt_type, 
     DalDmlIntf *dmi, uint8_t *vexternal, uint8_t *vex_if,
     InterfacePortMapInfo &flag);
#if 0
  /**
   * @brief Returns success if member of Boundary vlink
   *
   * @param[in]       ck_vbrif         ConfigKeyVal of the vbrif
   * @param[in]       dt_type        Configuration type 
   * @param[in/out]   ck_vlink       ConfigKeyVal of the vlink key formed
   * @param[in]       dmi            DB Connection
   * @param[out]      upll_rc_t      UPLL_RC_SUCCESS if member
   *                                 UPLL_RC_ERR_NO_SUCH_INSTANCE if not 
   *                                 UPLL_RC_SUCCESS on success
   *
   */
  upll_rc_t CheckIfMemberOfBoundaryVlink(ConfigKeyVal *ck_vbrif, 
                                 upll_keytype_datatype_t dt_type,
                                 ConfigKeyVal *&ck_vlink,
                                 DalDmlIntf *dmi);
#endif
  /**
     * @brief  Duplicates the input configkeyval including the key and val.  
     * based on the tbl specified.
     *
     * @param[in]  okey   Output Configkeyval - allocated within the function
     * @param[in]  req    Input ConfigKeyVal to be duplicated.
     * @param[in]  tbl    specifies if the val structure belongs to the main table/ controller table or rename table.
     *
     * @retval         UPLL_RC_SUCCESS      Successfull completion.
     * @retval         UPLL_RC_ERR_GENERIC  Failure case.
     **/
  upll_rc_t DupConfigKeyVal(ConfigKeyVal *&okey,
      ConfigKeyVal *&req, MoMgrTables tbl = MAINTBL);
  upll_rc_t GetMappedVbridges(uint8_t *logportid, DalDmlIntf *dmi,
             set<key_vnode_t>*sw_vbridge_set);
  upll_rc_t GetMappedInterfaces(key_vbr_t *vbr_key, DalDmlIntf *dmi,
             ConfigKeyVal *&ikey);
  /* create mo has to handle vex creation */

  /**
   * @Brief  Validates the syntax for KT_VBR_IF Keytype key structure.
   *
   * @param[in]  key_vbr_if  KT_VBR_IF key structure.
   * @param[in]  operation operation type.
   *
   * @retval  UPLL_RC_SUCCESS         validation succeeded.
   * @retval  UPLL_RC_ERR_CFG_SYNTAX  validation failed.
   */
  upll_rc_t ValidateVbrifKey(key_vbr_if *vbr_if_key, 
                           unc_keytype_operation_t operation = UNC_OP_INVALID);


  /* @brief         This is semantic check for KEY_VBR_IF key type
   *                in the update operation.
   *
   * @param[in/out] ikey     Pointer to the ConfigKeyVal Structure
   * @param[in]     datatype DB type.
   * @param[in]     dmi      Pointer to the DalDmlIntf(DB Interface)
   *
   * @retval  UPLL_RC_SUCCESS                    Completed successfully.
   * @retval  UPLL_RC_ERR_GENERIC                Generic failure.
   *
   **/


  upll_rc_t IsReferenced(ConfigKeyVal *ikey,
                         upll_keytype_datatype_t dt_type,
                         DalDmlIntf *dmi);

  upll_rc_t CreateAuditMoImpl(IpcReqRespHeader *req,
      ConfigKeyVal *ikey, DalDmlIntf *dmi, const char *ctrlr_id);
  upll_rc_t updateVbrIf(IpcReqRespHeader *req,
    ConfigKeyVal *ikey, DalDmlIntf *dmi);
  upll_rc_t ConverttoDriverPortMap(ConfigKeyVal *ck_port_map);
/**
    * @brief      Method to get a configkeyval of a specified keytype from an input configkeyval
    *
    * @param[in/out]  okey                 pointer to output ConfigKeyVal 
    * @param[in]      parent_key           pointer to the configkeyval from which the output 
    *                                      configkey val is initialized.
    *
    * @retval         UPLL_RC_SUCCESS      Successfull completion.
    * @retval         UPLL_RC_ERR_GENERIC  Failure case.
    */
  upll_rc_t GetChildConfigKey(ConfigKeyVal *&okey,
                      ConfigKeyVal *parent_key);


  upll_rc_t GetVbrIfValfromDB(ConfigKeyVal *ikey,
                              ConfigKeyVal *&ck_drv_vbr_if,
                              upll_keytype_datatype_t dt_type,
                              DalDmlIntf *dmi);
  /**
   * @brief  update controller candidate configuration with the difference in
   *      committed configuration between the UNC and the audited controller
   *
   * @param[in]  keytype     Specifies the keytype.
   * @param[in]  ctrlr_id    Specifies the controller Name.
   * @param[in]  session_id  Ipc client session id.
   * @param[in]  config_id   Ipc request header config id.
   * @param[in]  phase       Specifies the Controller name.
   * @param[in]  dmi         Pointer to DalDmlIntf class.
   *
   * @retval  UPLL_RC_SUCCESS                    Request successfully processed.
   * @retval  UPLL_RC_ERR_GENERIC                Generic error.
   * @retval  UPLL_RC_ERR_RESOURCE_DISCONNECTED  Resource is diconnected.
   * @retval  UPLL_RC_ERR_DB_ACCESS              DBMS access failure.
   * @retval  UPLL_RC_ERR_NO_SUCH_INSTANCE       Instance specified does not exist.
   * @retval  UPLL_RC_ERR_INSTANCE_EXISTS        Instance specified already exist.
   */
  upll_rc_t AuditUpdateController(unc_key_type_t keytype,
                                          const char *ctrlr_id,
                                          uint32_t session_id,
                                          uint32_t config_id,
                                          uuc::UpdateCtrlrPhase phase,
                                          bool *ctrlr_affected,
                                          DalDmlIntf *dmi);

/**
   * @brief  Update controller with the new created / updated / deleted configuration
   *                   between the Candidate and the Running configuration
   *
   * @param[in]  keytype                        Specifies the keytype
   * @param[in]  session_id                     Ipc client session id
   * @param[in]  config_id                      Ipc request header config id
   * @param[in]  phase                          Specifies the operation
   * @param[in]  dmi                            Pointer to DalDmlIntf class.
   * @param[out] affected_ctrlr_set             Returns the list of controller to
   *                                             which the command has been delivered.
   *
   * @retval  UPLL_RC_SUCCESS                    Request successfully processed.
   * @retval  UPLL_RC_ERR_GENERIC                Generic error.
   * @retval  UPLL_RC_ERR_RESOURCE_DISCONNECTED  Resource is diconnected.
   * @retval  UPLL_RC_ERR_DB_ACCESS              DBMS access failure.
   * @retval  UPLL_RC_ERR_NO_SUCH_INSTANCE       Instance specified does not exist.
   * @retval  UPLL_RC_ERR_INSTANCE_EXISTS        Instance specified already exist.
   */
   upll_rc_t TxUpdateController(unc_key_type_t keytype,
                                uint32_t session_id, uint32_t config_id,
                                uuc::UpdateCtrlrPhase phase,
                                set<string> *affected_ctrlr_set,
                                DalDmlIntf *dmi,
                                ConfigKeyVal **err_ckv);

   upll_rc_t PortStatusHandler(const char *ctrlr_name, 
                               const char *domain_name, 
                               const char *portid, 
                               bool oper_status, 
                               DalDmlIntf *dmi);

/**
    * @brief      Method to get a configkeyval of the parent keytype 
    *
    * @param[in/out]  okey           pointer to parent ConfigKeyVal 
    * @param[in]      ikey           pointer to the child configkeyval from 
    * which the parent configkey val is obtained.
    *
    * @retval         UPLL_RC_SUCCESS      Successfull completion.
    * @retval         UPLL_RC_ERR_GENERIC  Failure case.
    **/
  upll_rc_t GetParentConfigKey(ConfigKeyVal *&okey,
      ConfigKeyVal *parent_key);

};

}  // namespace kt_momgr
}  // namespace upll
}  // namespace unc
#endif