/*
 * Copyright (c) 2012-2013 NEC Corporation
 * All rights reserved.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#ifndef IPC_PFCDRV_IPC_ENUM_H_
#define IPC_PFCDRV_IPC_ENUM_H_

#include "unc/base.h"

/* enum for val_vtn structure */
typedef enum {
  PFCDRV_IDX_DOMAIN_ID_VTN = 0,
  PFCDRV_IDX_DESC_VTN
} pfcdrv_val_vtn_index_t;

/* enum for pfcdrv val vbrif structure */
typedef enum {
  PFCDRV_IDX_VAL_VBRIF = 0,
  PFCDRV_IDX_VEXT_NAME_VBRIF,
  PFCDRV_IDX_VEXTIF_NAME_VBRIF,
  PFCDRV_IDX_VLINK_NAME_VBRIF,
} pfcdrv_val_vbr_if_index_t;

/* index enumeration for val_vbr_if structure */
typedef enum {
  PFCDRV_IDX_DESC_VBRI = 0,
  PFCDRV_IDX_ADMIN_STATUS_VBRI,
  PFCDRV_IDX_PM_VBRI
} pfcdrv_upll_val_vbr_if_index;

/* enum for val port map structure */
typedef enum {
  PFCDRV_IDX_LOGICAL_PORT_ID_PM = 0,
  PFCDRV_IDX_VLAN_ID_PM,
  PFCDRV_IDX_TAGGED_PM,
} pfcdrv_upll_val_port_map_index_t;

/* specify vlan tagged */
typedef enum {
  PFCDRV_VLAN_UNTAGGED = 0,
  PFCDRV_VLAN_TAGGED
} pfcdrv_vlan_tagged_t;


/*index enumeration for pfcdrv_val_vbrif structure */
typedef enum {
  PFCDRV_IDX_INTERFACE_TYPE = 0,
  PFCDRV_IDX_VEXTERNAL_NAME_VBRIF,
  PFCDRV_IDX_VEXT_IF_NAME_VBRIF,
} pfcdrv_val_vbrif_vextif_index_t;

/* enum for val vbrif type */
typedef enum {
  PFCDRV_IF_TYPE_VBRIF = 0,
  PFCDRV_IF_TYPE_VEXTIF
} pfcdrv_val_vbrif_type_t;

/* index for pfcdriver val_flowfilter entry structure */
typedef enum {
  PFCDRV_IDX_FLOWFILTER_ENTRY_FFE = 0,
  PFCDRV_IDX_VAL_VBRIF_VEXTIF_FFE,
} pfcdrv_val_flowfilter_entry_index_t;

/* index for pfcdriver val_policingamp structure */
typedef enum {
  PFCDRV_IDX_VAL_POLICINGMAP_PM = 0,
  PFCDRV_IDX_VAL_VBRIF_VEXTIF_PM,
} pfcdrv_val_vbrif_policingmap_index_t;


#endif