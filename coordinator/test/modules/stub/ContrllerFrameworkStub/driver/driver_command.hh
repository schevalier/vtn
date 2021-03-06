/*
 * Copyright (c) 2013-2014 NEC Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#ifndef __CDF_DRIVER_COMMANDS_HH__
#define __CDF_DRIVER_COMMANDS_HH__

#include <pfc/ipc_struct.h>
#include <unc/unc_base.h>
#include <driver/controller_interface.hh>
#include <vector>


namespace unc {
namespace driver {
typedef struct {
} val_root_t;

class driver_command {
 public:
  virtual ~driver_command() {
  }
  virtual unc_key_type_t get_key_type()=0;
  virtual UncRespCode revoke(unc::driver::controller* ctr_ptr) {
    return UNC_RC_SUCCESS;
  }
  virtual UncRespCode fetch_config(unc::driver::controller* ctr,
                                       void* parent_key,
                    std::vector<unc::vtndrvcache::ConfigNode *>&) = 0;
};


class vtn_driver_command: public driver_command {
 public:
  UncRespCode create_cmd(key_vtn_t& keyvtn_, val_vtn_t& valvtn_,
                             controller*)  {
    return UNC_RC_SUCCESS;
  }

  UncRespCode update_cmd(key_vtn_t& keyvtn_, val_vtn_t& valvtn_,
                             controller*)  {
    return UNC_RC_SUCCESS;
  }

  UncRespCode delete_cmd(key_vtn_t& keyvtn_, val_vtn_t& valvtn_,
                             controller*) {
    return UNC_RC_SUCCESS;
  }

  UncRespCode validate_op(key_vtn_t& keyvtn_, val_vtn_t& valvtn_,
                              controller*, uint32_t op)  {
    return UNC_RC_SUCCESS;
  }

  unc_key_type_t get_key_type() {
    return UNC_KT_VTN;
  }
  UncRespCode fetch_config(
      unc::driver::controller* ctr,
      void* parent_key,
      std::vector<unc::vtndrvcache::ConfigNode *>&) {
    return UNC_RC_SUCCESS;
  }
};

class vbr_driver_command: public driver_command {
 public:
  UncRespCode create_cmd(key_vbr_t& keyvbr_, val_vbr_t& valvbr_,
                             unc::driver::controller*)  {
    return UNC_RC_SUCCESS;
  }


  UncRespCode update_cmd(key_vbr_t& keyvbr_, val_vbr_t& valvbr_,
                             unc::driver::controller*) {
    return UNC_RC_SUCCESS;
  }

  UncRespCode delete_cmd(key_vbr_t& keyvbr_, val_vbr_t& valvbr_,
                             unc::driver::controller*)  {
    return UNC_RC_SUCCESS;
  }

  UncRespCode validate_op(key_vbr_t& keyvbr_, val_vbr_t& valvbr_,
                              unc::driver::controller*, uint32_t op)  {
    return UNC_RC_SUCCESS;
  }

  unc_key_type_t get_key_type() {
    return UNC_KT_VBRIDGE;
  }
  UncRespCode fetch_config(
      unc::driver::controller* ctr,
      void* parent_key,
      std::vector<unc::vtndrvcache::ConfigNode *>&) {
    return UNC_RC_SUCCESS;
  }
};

class vbrif_driver_command: public driver_command {
 public:
  UncRespCode create_cmd(key_vbr_if_t& key,
           pfcdrv_val_vbr_if_t& val, unc::driver::controller *conn) {
    return UNC_RC_SUCCESS;
  }

  UncRespCode update_cmd(key_vbr_if_t& key,
           pfcdrv_val_vbr_if_t& val, unc::driver::controller *conn) {
    return UNC_RC_SUCCESS;
  }

  UncRespCode delete_cmd(key_vbr_if_t& key,
            pfcdrv_val_vbr_if_t& val, unc::driver::controller *conn) {
    return UNC_RC_SUCCESS;
  }

  UncRespCode validate_op(key_vbr_if_t& key,
            pfcdrv_val_vbr_if_t& val, unc::driver::controller* ctr,
                              uint32_t op) {
    return UNC_RC_SUCCESS;
  }

  unc_key_type_t get_key_type() {
    return UNC_KT_VBR_IF;
  }
  UncRespCode fetch_config(
      unc::driver::controller* ctr,
      void* parent_key,
      std::vector<unc::vtndrvcache::ConfigNode *>&) {
    return UNC_RC_SUCCESS;
  }
};

class controller_command: public driver_command {
 public:
  UncRespCode create_cmd(key_ctr_t& key,
               val_ctr_t & val, unc::driver::controller *conn) {
    return UNC_RC_SUCCESS;
  }

  UncRespCode update_cmd(key_ctr_t & key,
                   val_ctr_t& val, unc::driver::controller *conn) {
    return UNC_RC_SUCCESS;
  }

  UncRespCode delete_cmd(key_ctr_t & key,
                val_ctr_t & val, unc::driver::controller *conn) {
    return UNC_RC_SUCCESS;
  }

  UncRespCode validate_op(key_ctr_t & key,
                     val_ctr_t& val, unc::driver::controller* ctr,
                             uint32_t op) {
    return UNC_RC_SUCCESS;
  }

  unc_key_type_t get_key_type() {
    return UNC_KT_CONTROLLER;
  }
  UncRespCode fetch_config(
      unc::driver::controller* ctr,
      void* parent_key,
      std::vector<unc::vtndrvcache::ConfigNode *>&) {
    return UNC_RC_SUCCESS;
  }
};

    class vbrvlanmap_driver_command: public driver_command {
     public:
      /**
       * @brief    - Method to create Vbr VLAN-Map in the controller
       * @param[in]- key_vlan_map_t, val_vlan_map_t, controller*
       * @retval   - UNC_RC_SUCCESS/UNC_DRV_RC_ERR_GENERIC
       */
      UncRespCode create_cmd(key_vlan_map_t& key,
                                         val_vlan_map_t& val,
                                         unc::driver::controller *conn) {
    return UNC_RC_SUCCESS;
  }

      /**
       * @brief    - Method to update Vbr VLAN-Map in the controller
       * @param[in]- key_vlan_map_t, val_vlan_map_t, controller*
       * @retval   - UNC_RC_SUCCESS/UNC_DRV_RC_ERR_GENERIC
       */
      UncRespCode update_cmd(key_vlan_map_t& key,
                                         val_vlan_map_t& val,
                                         unc::driver::controller *conn) {
    return UNC_RC_SUCCESS;
  }

      /**
       * @brief    - Method to delete Vbr VLAN-Map in the controller
       * @param[in]- key_vlan_map_t, val_vlan_map_t, controller*
       * @retval   - UNC_RC_SUCCESS/UNC_DRV_RC_ERR_GENERIC
       */
      UncRespCode delete_cmd(key_vlan_map_t& key,
                                         val_vlan_map_t& val,
                                         unc::driver::controller *conn) {
    return UNC_RC_SUCCESS;
  }

      /**
       * @brief    - Method to return the Keytype
       * @param[in]- None
       * @retval   - unc_key_type_t - UNC_KT_VBR_VLANMAP
       */
      unc_key_type_t get_key_type() {
        return UNC_KT_VBR_VLANMAP;
      }
    };


class root_driver_command : public driver_command {
 public:
  UncRespCode
      create_cmd(key_root_t& key,
                 val_root_t & val,
                 unc::driver::controller *conn) {
        return UNC_RC_SUCCESS;
      }

  UncRespCode
      update_cmd(key_root_t& key,
                 val_root_t & val,
                 unc::driver::controller *conn) {
        return UNC_RC_SUCCESS;
      }

  UncRespCode
      delete_cmd(key_root_t& key,
                 val_root_t & val,
                 unc::driver::controller *conn) {
        return UNC_RC_SUCCESS;
      }

  UncRespCode
      validate_op(key_root_t& key,
                  unc::driver::val_root_t & val,
                  unc::driver::controller *conn,
                  uint32_t op) {
        return UNC_RC_SUCCESS;
      }

  UncRespCode
      read_root_child(std::vector<unc::vtndrvcache::ConfigNode*>&,
                      unc::driver::controller*) {
        if (set_root_child == 1) {
          return UNC_RC_NO_SUCH_INSTANCE;
        } else if (set_root_child == 2) {
          return UNC_DRV_RC_ERR_GENERIC;
        } else {
          return UNC_RC_SUCCESS;
        }
      }

  UncRespCode
      read_all_child(unc::vtndrvcache::ConfigNode*,
                     std::vector<unc::vtndrvcache::ConfigNode*>&,
                     unc::driver::controller*) {
        return UNC_RC_SUCCESS;
      }

  unc_key_type_t get_key_type() {
    return UNC_KT_ROOT;
  }
  UncRespCode fetch_config(
      unc::driver::controller* ctr,
      void* parent_key,
      std::vector<unc::vtndrvcache::ConfigNode *>&) {
    return UNC_RC_SUCCESS;
  }

  static uint32_t set_root_child;
};
}  // namespace driver
}  // namespace unc
#endif
