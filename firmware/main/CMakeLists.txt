# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS 
"ble/scli.c" 
"ble/misc.c" 
"ble/gatt_svr.c" 
"ble/ble_main.c" 
"ir/ir_nec_encoder.c" 
"ir/ir_nec.c" 
"main.c")
set(COMPONENT_ADD_INCLUDEDIRS "ble" "ir")

register_component()

